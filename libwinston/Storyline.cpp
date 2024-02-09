#include "Storyline.h"
#include "Util.h"

namespace winston
{
	
	Storyline::Task::Task()
		: _valid{false}
	{
	}

	void Storyline::Task::reset()
	{
		this->_valid = false;
	}

	const bool Storyline::Task::valid() const
	{
		return this->_valid;
	}

	Storyline::Storyline()
		: BasicCommand("Storyline")
		, tasks(), context()
	{
	}
	
	const State Storyline::execute()
	{
		auto& task = this->tasks.front();
		if (task->execute(this->shared_from_this(), context) == State::Finished)
		{
			this->context.push_back(task);
			this->tasks.pop_front();
		}

		return this->tasks.size() ? State::Running : State::Finished;
	}

	void Storyline::invent(const Invent invent, TextCallback callback)
	{
		this->tasks.clear();
		this->context.clear();
		this->tasks = invent();
		this->textCallback = textCallback;
	}

	const std::string Storyline::text() const
	{
		return this->textCallback(this->context);
	}

	const Result Storyline::reply(const Reply::Answer answer)
	{
		auto& task = this->tasks.front();
		auto replyTask = std::dynamic_pointer_cast<TaskReply>(task);
		if (replyTask)
			replyTask->reply(answer);
		else
			return Result::NotFound;

		return Result::OK;
	}

	const Result Storyline::reply(const std::string &answer)
	{
		if (std::string("none").compare(answer) == 0) return this->reply(Reply::Answer::Refresh);
		else if (std::string("refresh").compare(answer) == 0) return this->reply(Reply::Answer::Refresh);
		else if (std::string("cancel").compare(answer) == 0) return this->reply(Reply::Answer::Refresh);
			
		return Result::NotFound;

	}
	
	TaskRandomCars::TaskRandomCars(const RailCarShed& railCarShed)
		: Task(), railCarShed{ railCarShed }, _railCars{}
	{

	}

	const State TaskRandomCars::execute(const Storyline::Shared storyline, const Task::List & context)
	{
		this->_railCars.clear();
		if (!context.empty())
		{
			// remove all cars which are not compatible with the chosen locomotive
			auto taskLoco = storyline->getContext<TaskRandomLoco>();
			if (taskLoco != nullptr)
			{
				auto loco = taskLoco->locomotive();
				// Single => no cars
				if (loco->isType(Locomotive::Type::Single))
					return State::Finished;
			}
		}

		std::array<RailCar::Groups::Group, 4> groups = {
			RailCar::Groups::Person,
			RailCar::Groups::Goods,
			RailCar::Groups::Heavy,
			RailCar::Groups::ConstructionTrain,
		};

		auto chosen = random(groups);
		switch (chosen)
		{
		case RailCar::Groups::Person:
			this->findCars(chosen, 10, 3, 900);
			break;
		case RailCar::Groups::Goods:
			this->findCars(chosen, 20, 5, 900);
			break;
		case RailCar::Groups::ConstructionTrain:
			this->findCars(chosen, 30, 2, 900);
			break;
		case RailCar::Groups::Heavy:
			this->findCars(chosen, 0, 1, 900);
			break;
		}
		return State::Finished;
	}

	const std::string TaskRandomCars::text() const
	{
		return "Cars";
	}

	void TaskRandomCars::findCars(const RailCar::Groups::Group group, const unsigned char probOneOnly, const unsigned char maxNumCars, const Length maxLength)
	{
		// find all cars fitting the chosen type
		auto otherCars = filter(this->railCarShed, [&group](const RailCarShed::value_type& car) -> bool 
			{ 
				return car->is(group); 
			});

		// shuffle them and take 3
		otherCars = random(otherCars, std::rand() % 100 < probOneOnly ? 1 : maxNumCars);

		// restrict train length
		Length length = 0;
		otherCars.erase(std::remove_if(otherCars.begin(), otherCars.end(),
			[&length, maxLength](RailCarShed::value_type& car) -> const bool {
				if (length + car->length > maxLength)
					return true;

				length += car->length;
				return false;
			}), otherCars.end());

		this->_railCars.insert(this->_railCars.end(), otherCars.begin(), otherCars.end());
	}

	const std::list<RailCar::Shared> TaskRandomCars::railCars() const
	{
		return this->_railCars;
	}

	TaskRandomAction::TaskRandomAction()
		: _action{ Action::Drive }
	{

	}

	const State TaskRandomAction::execute(const Storyline::Shared storyline, const Task::List& context)
	{
		auto value = std::rand() % 100;
		if (value < 20)
			this->_action = Action::Assemble;
		else if (value < 80)
			this->_action = Action::Drive;
		else
			this->_action = Action::Stable;

		return State::Finished;
	}

	const std::string TaskRandomAction::text() const
	{
		switch (this->_action)
		{
		case Action::Assemble: return "zusammenstellen";
		case Action::Drive: return "fahren";
		case Action::Stable: return "abstellen";
		}

		return "???";
	}

	const TaskRandomAction::Action TaskRandomAction::action() const
	{
		return this->_action;
	}

	TaskRandomLoco::TaskRandomLoco(const LocomotiveShed& locoShed)
		: locoShed{ locoShed }
	{

	}

	const State TaskRandomLoco::execute(const Storyline::Shared storyline, const Task::List& context)
	{
		LocomotiveShed locos(this->locoShed);
		if (!context.empty())
		{
			// remove all cars which are not compatible with the chosen locomotive
			auto taskAction = storyline->getContext<TaskRandomAction>();
			if (taskAction != nullptr)
			{
				auto action = taskAction->action();
				// Only Shunting may do Assemble or Stable
				if (action == TaskRandomAction::Action::Assemble || action == TaskRandomAction::Action::Stable)
				{
					locos = filter(locos, [](const Locomotive::Shared& loco) -> const bool {
						return !loco->isType(Locomotive::Type::Shunting);
						});
				}
			}
		}
		this->_locomotive = random(locos);

		return State::Finished;
	}

	const std::string TaskRandomLoco::text() const
	{
		if (this->_locomotive)
			return this->_locomotive->name();
		else
			return "Locomotive ???";
	}

	const Locomotive::Shared TaskRandomLoco::locomotive() const
	{
		return this->_locomotive;
	}

	TaskRandomSection::TaskRandomSection(const SectionList& list)
		: list{ list }
	{

	}

	const State TaskRandomSection::execute(const Storyline::Shared storyline, const Task::List& context)
	{
		SectionList sections(this->list);
		if (!context.empty())
		{
			bool personRailCars = false;
			auto taskRailCars = storyline->getContext<TaskRandomCars>();
			if (taskRailCars != nullptr)
				personRailCars = taskRailCars->railCars().begin()->get()->is(RailCar::Groups::Person);

			auto taskAction = storyline->getContext<TaskRandomAction>();
			if (taskAction != nullptr)
			{
				auto action = taskAction->action();
				// Assemble on any section
				// Drive Person to Plattform
				// Drive others to Plattform or Sidings
				// Stable on Sidings
				if (action == TaskRandomAction::Action::Drive)
				{
					if (personRailCars)
						sections = filter(sections, [](const Section::Shared& section) -> const bool {
						return section->type != Section::Type::Platform;
							});
					else
						sections = filter(sections, [](const Section::Shared& section) -> const bool {
						return section->type != Section::Type::Platform && section->type != Section::Type::Siding;
							});

				}
				else if(action == TaskRandomAction::Action::Stable)
				{
					sections = filter(sections, [](const Section::Shared& section) -> const bool {
						return section->type != Section::Type::Siding;
						});
				}
			}
		}
		this->_section = random(sections);

		return State::Finished;
	}

	const std::string TaskRandomSection::text() const
	{
		if (this->_section)
			return this->_section->name;
		else
			return "Section ???";
	}

	const Section::Shared TaskRandomSection::section() const
	{
		return this->_section;
	}

	TaskReply::TaskReply(const Storyline::Reply::Callback callback)
		: Task(), Shared_Ptr<TaskReply>(), callback{ callback }, lastAnswer{Storyline::Reply::Answer::None}
	{

	}

	const State TaskReply::execute(const Storyline::Shared storyline, const Task::List& context)
	{
		if (this->lastAnswer == Storyline::Reply::Answer::None)
			return State::Running;
		else
			return State::Finished;
	}

	const std::string TaskReply::text() const
	{
		return "no text from TaskReply";
	}

	void TaskReply::reply(const Storyline::Reply::Answer answer)
	{
		this->lastAnswer = answer;
	}

	/*
	ConfirmationProvider::ConfirmationProvider()
		: Shared_Ptr<ConfirmationProvider>(), callback(nullptr)
	{

	}

	void ConfirmationProvider::confirm(const Answer answer)
	{
		if (this->callback)
			this->callback(answer);
	}

	void ConfirmationProvider::setCallback(ConfirmationProvider::Callback callback)
	{
		this->callback = callback;
	}

	void ConfirmationProvider::clearCallbackTask()
	{
		this->callback = nullptr;
	}

	TaskRandomLoco::TaskRandomLoco(const Locomotive::Types types)
		: Storyline::Task(), types(types)
	{

	}
	
	const State TaskRandomLoco::execute()
	{
		return State::Finished;
	}
	
	const std::string TaskRandomLoco::text() const
	{
		return "Random Locomotive";
	}

	TaskRandomTrack::TaskRandomTrack(const Section::Type type)
		: Storyline::Task(), type(type)
	{
		
	}

	const State TaskRandomTrack::execute()
	{
		return State::Finished;
	}

	const std::string TaskRandomTrack::text() const
	{
		return "Random Track";
	}

	TaskConfirm::TaskConfirm(ConfirmationProvider::Shared confirmationProvider, Display::Shared display, const Task::Shared a, const std::string question, const Task::Shared b, const MaybeCallback maybe)
		: Storyline::Task(), registered(false), confirmationProvider(confirmationProvider), display(display), a(a), question(question), b(b), maybe(maybe), answer(ConfirmationProvider::Answer::None)
	{

	}
	const State TaskConfirm::execute()
	{
		if (!this->registered)
		{
			this->confirmationProvider->setCallback([&](const ConfirmationProvider::Answer answer) {
				this->answer = answer;
			});
			std::vector<std::string> data;
			data.emplace_back(this->text());
			display->send(data);
			this->registered = true;
		}
		switch (this->answer)
		{
		default:
		case ConfirmationProvider::Answer::None: return State::Running;
		case ConfirmationProvider::Answer::Yes: return State::Finished;
		case ConfirmationProvider::Answer::No: return State::Aborted;
		case ConfirmationProvider::Answer::Maybe: return this->maybe();
		}
	}

	const std::string TaskConfirm::text() const
	{
		return winston::build(a->text(), " ", question, " ", b->text(), "?");
	}

	DisplayLog::DisplayLog()
	{

	}

	const winston::Result DisplayLog::send(const std::vector<DataType> data)
	{
		for (auto& line : data)
			winston::hal::text(winston::build(line, "\n"));
		return winston::Result::OK;
	}
	*/
};