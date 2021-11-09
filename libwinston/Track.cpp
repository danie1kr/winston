#include <algorithm>

#include "Track.h"
#include "HAL.h"
#include "Util.h"

#include "magic_enum.hpp"

namespace winston
{
	Track::Track(const std::string name, Length trackLength) : Shared_Ptr<Track>(), _name(name), _block(0), trackLength(trackLength)
	{
	}

	const std::string Track::ConnectionToString(const Connection connection)
	{
		auto name = std::string(magic_enum::enum_name(connection));
		std::transform(name.begin(), name.end(), name.begin(), ::tolower);
		return name;
	}

	const Track::Connection Track::ConnectionFromString(const std::string connection)
	{
		std::string upper(connection.size(), ' ');
		std::transform(connection.begin(), connection.end(), upper.begin(), ::toupper);
		auto c = magic_enum::enum_cast<Connection>(upper);
		return c.value();
	}
	Track::Shared Track::connect(const Connection local, Track::Shared& to, const Connection remote)
	{
		return this->connectTo(local, nullptr, to, remote, nullptr);
	}

	Track::Shared Track::connect(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote)
	{
		return this->connectTo(local, guardingLocalSignalFactory, to, remote, nullptr);
	}

	Track::Shared Track::connect(const Connection local, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory)
	{
		return this->connectTo(local, nullptr, to, remote, guardingRemoteSignalFactory);
	}

	Track::Shared Track::connect(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory)
	{
		return this->connectTo(local, guardingLocalSignalFactory, to, remote, guardingRemoteSignalFactory);
	}

	void Track::block(const Address address)
	{
		this->_block = address;
	}

	const Address Track::block() const
	{
		return this->_block;
	}

	void Track::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		hal::fatal("Cannot attach signal");
	}

	Signal::Shared Track::signalFacing(const Connection facing)
	{
		return nullptr;
	}

	Signal::Shared Track::signalGuarding(const Connection guarding)
	{
		return nullptr;
	}

	Result Track::validateSingle(Track::Shared track)
	{
		if (track->block() == 0)
			return Result::ValidationFailed;


		std::set<Track::Shared> others;
		track->collectAllConnections(others);

		bool foundMe = false;
		for (auto other : others)
		{
			foundMe = other.get() == this;
			if (foundMe == true)
				return Result::OK;
		}

		return Result::ValidationFailed;
	}

	const Length Track::length()
	{
		return this->trackLength;
	}
	
	const std::string Track::name()
	{
		return this->_name;
	}

	Bumper::Bumper(const std::string name)
		: Track(name), Shared_Ptr<Bumper>(), a()
	{

	}

	bool Bumper::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::DeadEnd;
	}

	Track::Shared Bumper::on(const Connection connection) const
	{
		if (connection == Connection::A)
			return a;
		return nullptr;
	}

	bool Bumper::traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const
	{
		if(this->has(connection) && 
			((leavingOnConnection && connection == Connection::A) ||
			(!leavingOnConnection && connection == Connection::DeadEnd)))
		{
			onto = a;
			return true;
		}

		onto.reset();
		return false;

	}

	void Bumper::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
	}

	const Track::Connection Bumper::whereConnects(Track::Shared& other) const
	{
		if (other == a)
			return Connection::A;
		else
			return Connection::DeadEnd;
	}

	const Track::Connection Bumper::otherConnection(const Connection connection) const
	{
		return connection == Connection::A ? Connection::DeadEnd : Connection::A;
	}

	Track::Shared Bumper::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::A)
		{
			a = to;
			Track::Shared that = this->shared_from_this();
			if (viceVersa)
				to->connectTo(remote, nullptr, that, local, nullptr, false);
			if (guardingSignalFactory)
				this->attachSignal(guardingSignalFactory(that, local), local);
			if(guardingRemoteSignalFactory)
				to->attachSignal(guardingRemoteSignalFactory(to, remote), remote);
		}
		else
			error("Bumper::connect, local not A");
		return to;
	}


	const Result Bumper::validate()
	{
		return this->validateSingle(a);
	}

	void Bumper::connections(Track::Shared& onA)
	{
		onA = this->a;
	}

	const Track::Type Bumper::type()
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

	void Turnout::connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC)
	{
		onA = a;
		onB = b;
		onC = c;
	}

	Rail::Rail(const std::string name)
		: Track(name), Shared_Ptr<Rail>(), a(), b()
	{

	}

	bool Rail::has(const Connection connection) const
	{
		return connection == Connection::A || connection == Connection::B;
	}

	Track::Shared Rail::on(const Connection connection) const
	{
		if (connection == Connection::A)
			return a;
		else if (connection == Connection::B)
			return b;
		return nullptr;
	}

	bool Rail::traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const
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

	void Rail::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
		tracks.insert(b);
	}

	const Track::Connection Rail::whereConnects(Track::Shared& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else
			return Track::Connection::DeadEnd;
	}

	const Track::Connection Rail::otherConnection(const Connection connection) const
	{
		return connection == Connection::A ? Connection::B : Connection::A;
	}

	Track::Shared Rail::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::C)
			error("Rail::connect, local not A or B");
		else
		{
			if (local == Connection::A)
				a = to;
			else if (local == Connection::B)
				b = to;
			
			Track::Shared that = this->shared_from_this();
			if (viceVersa)
				to->connectTo(remote, nullptr, that, local, nullptr, false);
			if (guardingSignalFactory)
				this->attachSignal(guardingSignalFactory(that, local), local);
			if (guardingRemoteSignalFactory)
				to->attachSignal(guardingRemoteSignalFactory(to, remote), remote);
		}
		return to;
	}

	const Result Rail::validate()
	{
		return this->validateSingle(a) == Result::OK && this->validateSingle(b) == Result::OK ? Result::OK : Result::ValidationFailed;
	}

	const Track::Type Rail::type()
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

	void Rail::connections(Track::Shared& onA, Track::Shared& onB)
	{
		onA = this->a;
		onB = this->b;
	}

	Turnout::Turnout(const std::string name, const Callback callback, const bool leftTurnout)
		: Track(name), Shared_Ptr<Turnout>(), callback(callback), trackLengthCalculator(nullptr), leftTurnout(leftTurnout), dir(Direction::A_B), a(), b(), c()
	{

	}

	Turnout::Turnout(const std::string name, const Callback callback, const TrackLengthCalculator trackLengthCalculator, const bool leftTurnout)
		: Track(name), Shared_Ptr<Turnout>(), callback(callback), trackLengthCalculator(trackLengthCalculator), leftTurnout(leftTurnout), dir(Direction::A_B), a(), b(), c()
	{

	}

	bool Turnout::has(const Connection connection) const
	{
		return connection != Connection::DeadEnd;
	}

	Track::Shared Turnout::on(const Connection connection) const
	{
		if (connection == Connection::A)
			return a;
		else if (connection == Connection::B)
			return b;
		else if (connection == Connection::C)
			return c;
		return nullptr;
	}

	bool Turnout::traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const
	{
		if (!this->has(connection) || this->dir == Direction::Changing)
		{
			onto.reset();
			return false;
		}
		if (leavingOnConnection)
		{
			/*if ((this->dir == Direction::A_B && connection == Connection::C) || (this->dir == Direction::A_C && connection == Connection::B))
			{
				onto.reset();
				return false;
			}*/

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

	void Turnout::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
		tracks.insert(b);
		tracks.insert(c);
	}

	const Track::Connection Turnout::whereConnects(Track::Shared& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else if (c == other)
			return Track::Connection::C;
		else
			return Track::Connection::DeadEnd;
	}

	const Track::Connection Turnout::otherConnection(const Connection connection) const
	{
		if (connection == Connection::A)
			return this->dir == Direction::A_B ? Connection::B : Connection::C;
		else
			return Connection::A;
	}

	Track::Shared Turnout::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::A)
			a = to;
		else if (local == Connection::B)
			b = to;
		else
			c = to;

		if (viceVersa)
		{
			Track::Shared that = this->shared_from_this();
			to->connectTo(remote, nullptr, that, local, nullptr, false);
		}
		if (guardingSignalFactory)
			hal::fatal("Cannot attach signal to turnout");
		if (guardingRemoteSignalFactory)
			to->attachSignal(guardingRemoteSignalFactory(to, remote), remote);
		return to;
	}

	const Result Turnout::validate()
	{
		return this->validateSingle(a) == Result::OK && this->validateSingle(b) == Result::OK && this->validateSingle(c) == Result::OK ? Result::OK : Result::ValidationFailed;
	}

	const Track::Type Turnout::type()
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

		return state;
	}

	const Turnout::Direction Turnout::direction()
	{
		return this->dir;
	}

	const Turnout::Direction Turnout::otherDirection(const Direction current)
	{
		if (current == Direction::A_B)
			return Direction::A_C;
		else
			return Direction::A_B;
	}

	const Turnout::Direction Turnout::fromConnection(const Connection connection)
	{
		if (connection == Connection::B)
			return Direction::A_B;
		else if (connection == Connection::C)
			return Direction::A_C;
		
		hal::fatal("cannot derive direction from connection a");
		return Direction::Changing;
	}

	void Turnout::fromDirection(Track::Shared& a, Track::Shared& other) const {
		a = this->a;
		other = this->dir == Direction::Changing ? nullptr : (this->dir == Direction::A_B ? this->b : this->c);
	}

	const Track::Connection Turnout::fromDirection() const
	{
		return this->dir == Direction::Changing ? Connection::DeadEnd : (this->dir == Direction::A_B ? Connection::B : Connection::C);
	}

	const Length Turnout::length()
	{
		return this->trackLengthCalculator ? this->trackLengthCalculator(this->dir) : 0;
	}
}