#include "Rail.h"
#include "Util.h"

namespace winston
{
	Section::Section() : Shared_Ptr<Section>()
	{
	}

	Result Section::validateSingle(Section::Shared section)
	{
		std::set<Section::Shared> others;
		section->collectAllConnections(others);

		bool foundMe = false;
		for (auto other : others)
		{
			foundMe = other.get() == this;
			if (foundMe == true)
				return Result::OK;
		}

		return Result::ValidationFailed;
	}

	Bumper::Bumper()
		: Section(), Shared_Ptr<Bumper>(), a()
	{

	}

	/*Section::Shared Bumper::make()
	{
		return std::make_shared<Bumper>();
	}*/

	bool Bumper::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::DeadEnd;
	}

	bool Bumper::traverse(const Connection from, Section::Shared& onto) const
	{
		if (!this->has(from))
		{
			onto.reset();
			return false;
		}
		if (from == Connection::DeadEnd)
		{
			onto = a;
			return true;
		}
		else
		{
			onto.reset();
			return false;
		}
	}

	void Bumper::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
	}

	Section::Shared Bumper::connect(const Connection local, Section::Shared& to, const Connection remote, bool viceVersa)
	{
		if (local == Connection::A)
		{
			a = to;
			if (viceVersa)
			{
				Section::Shared that = this->shared_from_this();
				to->connect(remote, that, local, false);
			}
		}
		else
			error("Bumper::connect, local not A");
		return to;
	}

	const Result Bumper::validate()
	{
		return this->validateSingle(a);
	}

	void Bumper::connections(Section::Shared& onA)
	{
		onA = this->a;
	}

	const Section::Type Bumper::type()
	{
		return Type::Bumper;
	}

	void Turnout::connections(Section::Shared& onA, Section::Shared& onB, Section::Shared& onC)
	{
		onA = a;
		onB = b;
		onC = c;
	}

	Rail::Rail()
		: Section(), Shared_Ptr<Rail>(), a(), b()
	{

	}

	/*Section::Shared Rail::make()
	{
		return std::make_shared<Rail>();
	}*/

	bool Rail::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::B;
	}

	bool Rail::traverse(const Connection from, Section::Shared& onto) const
	{
		if (!this->has(from))
		{
			onto.reset();
			return false;
		}
		onto = from == Connection::A ? b : a;
		return true;
	}

	void Rail::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
		sections.insert(b);
	}

	Section::Shared Rail::connect(const Connection local, Section::Shared& to, const Connection remote, bool viceVersa)
	{
		if (local == Connection::C)
			error("Rail::connect, local not A or B");
		else
		{
			if (local == Connection::A)
				a = to;
			else if (local == Connection::B)
				b = to;
			
			if (viceVersa)
			{
				Section::Shared that = this->shared_from_this();
				to->connect(remote, that, local, false);
			}
		}
		return to;
	}

	const Result Rail::validate()
	{
		return this->validateSingle(a) == Result::OK && this->validateSingle(b) == Result::OK ? Result::OK : Result::ValidationFailed;
	}

	const Section::Type Rail::type()
	{
		return Type::Rail;
	}

	void Rail::connections(Section::Shared& onA, Section::Shared& onB)
	{
		onA = this->a;
		onB = this->b;
	}

	Turnout::Turnout(const Callback callback, const bool leftTurnout)
		: Section(), Shared_Ptr<Turnout>(), callback(callback), leftTurnout(leftTurnout), dir(Direction::A_B), a(), b(), c()
	{

	}

	/*Section::Shared Turnout::make(const Callback callback, const bool leftTurnout)
	{
		return std::make_shared<Turnout>(callback, leftTurnout);
	}*/

	bool Turnout::has(const Connection connection) const
	{
		return connection != Connection::DeadEnd;
	}

	bool Turnout::traverse(const Connection from, Section::Shared& onto) const
	{
		if (!this->has(from) || this->dir == Direction::Changing)
		{
			onto.reset();
			return false;
		}
		if (from == Connection::A)
		{
			onto = this->dir == Direction::A_B ? b : c;
			return true;
		}
		else if ((from == Connection::B && this->dir == Direction::A_B) ||
			(from == Connection::C && this->dir == Direction::A_C))
		{
			onto = a;
			return true;
		}
		else
			return false;
	}

	void Turnout::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
		sections.insert(b);
		sections.insert(c);
	}

	Section::Shared Turnout::connect(const Connection local, Section::Shared& to, const Connection remote, bool viceVersa)
	{
		if (local == Connection::A)
			a = to;
		else if (local == Connection::B)
			b = to;
		else
			c = to;

		if (viceVersa)
		{
			Section::Shared that = this->shared_from_this();
			to->connect(remote, that, local, false);
		}
		return to;
	}

	const Result Turnout::validate()
	{
		return this->validateSingle(a) == Result::OK && this->validateSingle(b) == Result::OK && this->validateSingle(c) == Result::OK ? Result::OK : Result::ValidationFailed;
	}

	const Section::Type Turnout::type()
	{
		return Type::Turnout;
	}

	const State Turnout::startChangeTo(const Direction direction)
	{
		if (this->dir == direction)
			return State::Finished;

		State state = this->callback(this->shared_from_this(), direction);
		this->dir = Direction::Changing;

		return state;
	}

	const State Turnout::startToggle()
	{
		return this->startChangeTo(Turnout::otherDirection(this->dir));
	}

	const State Turnout::finalizeChangeTo(const Direction direction)
	{
		if (this->dir == direction)
			return State::Finished;

		State state = this->callback(this->shared_from_this(), direction);
		this->dir = direction;

		return State::Finished;
	}

	const Turnout::Direction Turnout::direction()
	{
		return this->dir;
	}

	/*const Task::State Turnout::toggle()
	{
		Direction newDirection = otherDirection(this->direction);
		Task::State state = this->callback(newDirection);
		if (state == Task::State::Finished)
			this->direction = newDirection;

		return state;
		
	}

	const Task::State Turnout::toggle(const Direction direction)
	{
		if (this->direction != direction)
			return this->toggle();

		return Task::State::Finished;
	}*/

	Turnout::Direction Turnout::otherDirection(const Direction current)
	{
		if (current == Direction::A_B)
			return Direction::A_C;
		else
			return Direction::A_B;
	}
}