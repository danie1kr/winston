#include "Rail.h"
#include "HAL.h"
#include "Util.h"

namespace winston
{
	Section::Section() : Shared_Ptr<Section>()
	{
	}

	void Section::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		hal::fatal("Cannot attach signal");
	}

	Signal::Shared Section::signalFacing(const Connection facing)
	{
		return nullptr;
	}

	Signal::Shared Section::signalGuarding(const Connection guarding)
	{
		return nullptr;
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

	bool Bumper::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::DeadEnd;
	}

	void Bumper::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
	}

	const Section::Connection Bumper::whereConnects(Section::Shared& other) const
	{
		if (other == a)
			return Connection::A;
		else
			return Connection::DeadEnd;
	}

	const Section::Connection Bumper::otherConnection(const Connection connection) const
	{
		return connection == Connection::A ? Connection::DeadEnd : Connection::A;
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

	void Bumper::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		switch (guarding)
		{
		case Connection::A: this->signals[0] = signal; return; break;
		case Connection::DeadEnd: this->signals[1] = signal; return; break;
		}

		hal::fatal("cannot put signal");
	}

	Signal::Shared Bumper::signalGuarding(const Connection guarding)
	{
		return signals[guarding == Connection::A ? 0 : 1];
	}

	Signal::Shared Bumper::signalFacing(const Connection facing)
	{
		return signals[facing == Connection::A ? 1 : 0];
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

	bool Rail::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::B;
	}

	bool Rail::traverse(const Connection connection, Section::Shared& onto, bool leavingOnConnection) const
	{
		if (!this->has(connection))
		{
			onto.reset();
			return false;
		}
		if (leavingOnConnection)
			onto = connection == Connection::A ? a : b;
		else
			onto = connection == Connection::A ? b : a;
		return true;
	}

	void Rail::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
		sections.insert(b);
	}

	const Section::Connection Rail::whereConnects(Section::Shared& other) const
	{
		if (a == other)
			return Section::Connection::A;
		else if (b == other)
			return Section::Connection::B;
		else
			return Section::Connection::DeadEnd;
	}

	const Section::Connection Rail::otherConnection(const Connection connection) const
	{
		return connection == Connection::A ? Connection::B : Connection::A;
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

	void Rail::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		switch (guarding)
		{
		case Connection::A: this->signals[0] = signal; return; break;
		case Connection::B: this->signals[1] = signal; return; break;
		}

		hal::fatal("cannot put signal");
	}

	Signal::Shared Rail::signalGuarding(const Connection guarding)
	{
		return this->signals[guarding == Connection::A ? 0 : 1];
	}

	Signal::Shared Rail::signalFacing(const Connection facing)
	{
		return signals[facing == Connection::A ? 1 : 0];
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

	bool Turnout::has(const Connection connection) const
	{
		return connection != Connection::DeadEnd;
	}

	bool Turnout::traverse(const Connection connection, Section::Shared& onto, bool leavingOnConnection) const
	{
		if (!this->has(connection) || this->dir == Direction::Changing)
		{
			onto.reset();
			return false;
		}
		if (leavingOnConnection)
		{
			switch (connection)
			{
			case Connection::A: onto = a; return true; break;
			case Connection::B: onto = b; return true; break;
			case Connection::C: onto = c; return true; break;
			}
		}
		else
		{
			if (connection == Connection::A)
			{
				onto = this->dir == Direction::A_B ? b : c;
				return true;
			}
			else if ((connection == Connection::B && this->dir == Direction::A_B) ||
				(connection == Connection::C && this->dir == Direction::A_C))
			{
				onto = a;
				return true;
			}
		}
		return false;
	}

	void Turnout::collectAllConnections(std::set<Section::Shared>& sections) const
	{
		sections.insert(a);
		sections.insert(b);
		sections.insert(c);
	}

	const Section::Connection Turnout::whereConnects(Section::Shared& other) const
	{
		if (a == other)
			return Section::Connection::A;
		else if (b == other)
			return Section::Connection::B;
		else if (c == other)
			return Section::Connection::C;
		else
			return Section::Connection::DeadEnd;
	}

	const Section::Connection Turnout::otherConnection(const Connection connection) const
	{
		if (connection == Connection::A)
			return this->dir == Direction::A_B ? Connection::B : Connection::C;
		else
			return Connection::A;
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
		if (this->dir == direction || this->dir == Direction::Changing)
			return State::Finished;

		this->dir = Direction::Changing;
		return this->callback(this->shared_from_this(), this->dir);
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

	Turnout::Direction Turnout::otherDirection(const Direction current)
	{
		if (current == Direction::A_B)
			return Direction::A_C;
		else
			return Direction::A_B;
	}
}