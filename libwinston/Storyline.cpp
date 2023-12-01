#include "Storyline.h"

namespace winston
{
	Storyline::Task::Task()
		: Shared_Ptr<Task>()
	{
	}

	const State Storyline::Task::abort() const
	{
		return State::Aborted;
	}


	Storyline::Storyline()
		: BasicCommand("Storyline")
		, tasks()
	{
	}

	const State Storyline::execute()
	{
		auto& task = this->tasks.front();
		if (task->execute() == State::Finished)
			this->tasks.pop_front();

		return this->tasks.size() ? State::Running : State::Finished;
	}

	void Storyline::queue(Task::Shared task)
	{
		this->tasks.push_back(task);
	}

	void Storyline::immediate(Task::Shared task)
	{
		this->tasks.push_front(task);
	}

	const void Storyline::invent(const Invent invent) const
	{
		invent();
	}

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
};