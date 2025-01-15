#include <algorithm>

#include "HAL.h"
#include "Util.h"
#include "Track.h"
#include "NextSignals.h"

namespace winston
{
	Track::NextSignalProvider::NextSignalProvider(const Distance distance, const Signal::Shared signal, const Track::Const track, const Track::Connection connection)
		: Shared_Ptr<NextSignalProvider>(), distance(distance), signal(signal), track(track), connection(connection), nextTurnout()
	{

	}

	Track::NextSignalProvider::NextSignalProvider(const Distance distance, const NextTurnout::Const nextTurnout)
		: Shared_Ptr<NextSignalProvider>(), distance(distance), signal(), track(), connection(Connection::DeadEnd), nextTurnout(nextTurnout)
	{

	}

	Track::NextSignalProvider::NextTurnout::NextTurnout(const Track::Const turnout, const Track::Connection entered)
	: Shared_Ptr<NextTurnout>(), turnout(turnout), entered(entered)
	{
	}


	Track::Track(const std::string name, const Id index, const Length trackLength) 
		: Shared_Ptr<Track>()
		, index(index)
		, _name(name)
		, _section(0)
		, _segment(0)
		, trackLength(trackLength)
	{
	}

	const std::string Track::ConnectionToString(const Connection connection)
	{
		switch (connection)
		{
		case Connection::A: return "a";
		case Connection::B: return "b";
		case Connection::C: return "c";
		case Connection::D: return "d";
		default:
		case Connection::DeadEnd: return "deadend";
		}
	}

	const Track::Connection Track::ConnectionFromString(const std::string connection)
	{
		if (connection == "A" || connection == "a") return Connection::A;
		if (connection == "B" || connection == "b") return Connection::B;
		if (connection == "C" || connection == "c") return Connection::C;
		if (connection == "D" || connection == "d") return Connection::D;
		return Connection::DeadEnd;
	}

	std::string build(const winston::Track::Connection first)
	{
		return winston::Track::ConnectionToString(first);
	}

	Track::Shared Track::connect(const Connection local, Track::Shared& to, const Connection remote)
	{
		return this->connectTo(local, nullptr, to, remote, nullptr);
	}

	void Track::section(const Address address)
	{
		this->_section = address;
	}

	const Address Track::section() const
	{
		return this->_section;
	}

	void Track::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		hal::fatal("Cannot attach signal");
	}

	Signal::Shared Track::signalFacing(const Connection facing) const
	{
		return nullptr;
	}

	Signal::Shared Track::signalGuarding(const Connection guarding) const
	{
		return nullptr;
	}

	const Result Track::validateSingle(Track::Shared track) const
	{
		if(!track)
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

	const Length Track::length() const
	{
		return this->trackLength;
	}
	
	const std::string Track::name() const
	{
		return this->_name;
	}

	const Id Track::segment() const
	{
		return this->_segment;
	}

	void Track::segment(const Id segment)
	{
		this->_segment = segment;
	}
	const bool Track::traverseToNextSegment(const Track::Connection connection, const bool leavingOnConnection, Track::Const& onto, Track::Connection &ontoConnection) const
	{
		auto con = leavingOnConnection ? connection : this->otherConnection(connection);

		auto current = this->const_from_this();
		while (current->traverse(con, onto, true))
		{
			con = onto->whereConnects(current);
			ontoConnection = con;
			con = onto->otherConnection(con);

			if (onto->segment() != current->segment())
			{
				return true;
			}

			current = onto;
		}

		return false;
	}

	const bool Track::nextSignal(const Connection leaving, const Signal::Pass pass, Signal::Shared &signal, Track::Const &track, Track::Connection &guarding, Distance& distance) const
	{
		auto current = this->const_from_this();
		auto connection = leaving;
		while (true)
		{
			if (const auto provider = current->getNextSignalProvider(connection, pass))
			{
				if (provider->signal)
				{
					// we found a next signal, so return this
					signal = provider->signal;
					track = provider->track;
					guarding = provider->connection;
					distance += provider->distance;
					return true;
					//return NextSignal::make(signal, current, connection, provider->distance, pass);
				}
				else if (const auto nextTurnout = provider->nextTurnout)
				{
					// we found a turnout, so we need to see where it leads to and continue there
					//connection = current->whereConnects(nextTurnout->turnout);
					//current = nextTurnout->turnout;
					connection = nextTurnout->entered;
					current = nextTurnout->turnout;

					// traverse the current turnout to the other side
					connection = current->otherConnection(connection);
				}
				else
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}

		/*
		if (const auto signal = std::get_if<Signal::Shared>(&provider->signalOrTurnout))
		{
			return *signal;
		}
		else if (const auto track = std::get_if<Track::Const>(&provider->signalOrTurnout))
		{
			return std::get<Signal::Shared>(provider->signalOrTurnout);
		}
		else
		*/
		return false;
	}

	Bumper::Bumper(const std::string name, const Id index, const Length tracklength)
		: Track(name, index, tracklength), Shared_Ptr<Bumper>(), a()
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

	const bool Bumper::traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const
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

	const bool Bumper::canTraverse(const Connection entering) const
	{
		return entering == Connection::A;
	}

	void Bumper::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
	}
	/*
	const Track::Connection Bumper::whereConnects(const Track::Shared& other) const
	{
		if (other == a)
			return Connection::A;
		else
			return Connection::DeadEnd;
	}*/

	const Track::Connection Bumper::whereConnects(const Track::Const& other) const
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

	void Bumper::eachConnection(ConnectionCallback callback)
	{
		callback(*this, Connection::A);
		callback(*this, Connection::DeadEnd);
	}

	Track::Shared Bumper::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::A)
		{
			if (a)
				error("Bumper::connect on " + this->name() + ", A already connected");

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
			error("Bumper::connect on " + this->name() + ", local not A");
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

	const Track::Type Bumper::type() const
	{
		return Type::Bumper;
	}

	void Bumper::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		switch (guarding)
		{
		case Connection::A: this->signals[0] = signal; return; break;
		case Connection::DeadEnd: this->signals[1] = signal; return; break;
		default:
			hal::fatal("cannot put signal");
			return;
		}

	}

	Signal::Shared Bumper::signalGuarding(const Connection guarding) const
	{
		return signals[guarding == Connection::A ? 0 : 1];
	}

	Signal::Shared Bumper::signalFacing(const Connection facing) const
	{
		return signals[facing == Connection::A ? 1 : 0];
	}

	const size_t Bumper::nextSignalsIndex(const Connection connection, const Signal::Pass pass) const
	{
		switch (connection)
		{
		case Connection::A:
			return pass == Signal::Pass::Facing ? 0 : 1;
		case Connection::DeadEnd:
			return 2;
		default:
			return 0;
		}
	}

	void Bumper::setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider)
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			this->nextSignals[idx] = nextSignalProvider;
	}

	Track::NextSignalProvider::Shared Bumper::getNextSignalProvider(const Connection connection, const Signal::Pass pass) const
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			return this->nextSignals[idx];

		return nullptr;
	}

	Rail::Rail(const std::string name, const Id index, const Length tracklength)
		: Track(name, index, tracklength), Shared_Ptr<Rail>(), a(), b()
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

	const bool Rail::traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const
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

	const bool Rail::canTraverse(const Connection entering) const
	{
		return true;
	}

	void Rail::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
		tracks.insert(b);
	}
	/*
	const Track::Connection Rail::whereConnects(const Track::Shared& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else
			return Track::Connection::DeadEnd;
	}*/

	const Track::Connection Rail::whereConnects(const Track::Const& other) const
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
	void Rail::eachConnection(ConnectionCallback callback)
	{
		callback(*this, Connection::A);
		callback(*this, Connection::B);
	}

	Track::Shared Rail::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::C)
			error("Rail::connect on " + this->name() + ", local not A or B");
		else
		{
			if (local == Connection::A)
			{
				if (a)
					error("Rail::connect on " + this->name() + ", A already connected");
				a = to;
			}
			else if (local == Connection::B)
			{
				if (b)
					error("Rail::connect on " + this->name() + ", B already connected");
				b = to;
			}
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

	const Track::Type Rail::type() const
	{
		return Type::Rail;
	}

	void Rail::attachSignal(Signal::Shared signal, const Connection guarding)
	{
		switch (guarding)
		{
		case Connection::A: this->signals[0] = signal; return; break;
		case Connection::B: this->signals[1] = signal; return; break;
		default:
			hal::fatal("cannot put signal");
		}

	}

	Signal::Shared Rail::signalGuarding(const Connection guarding) const
	{
		return this->signals[guarding == Connection::A ? 0 : 1];
	}

	Signal::Shared Rail::signalFacing(const Connection facing) const
	{
		return signals[facing == Connection::A ? 1 : 0];
	}

	void Rail::connections(Track::Shared& onA, Track::Shared& onB)
	{
		onA = this->a;
		onB = this->b;
	}

	const size_t Rail::nextSignalsIndex(const Connection connection, const Signal::Pass pass) const
	{
		switch (connection)
		{
		case Connection::A:
			return pass == Signal::Pass::Facing ? 0 : 1;
		case Connection::B:
			return pass == Signal::Pass::Facing ? 2 : 3;
		default:
			return 0;
		}
	}

	void Rail::setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider)
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			this->nextSignals[idx] = nextSignalProvider;
	}

	Track::NextSignalProvider::Shared Rail::getNextSignalProvider(const Connection connection, const Signal::Pass pass) const
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			return this->nextSignals[idx];

		return nullptr;
	}

	const std::string Turnout::DirectionToString(const Direction direction)
	{
		switch (direction)
		{
		case Direction::A_B: return std::string("A_B");
		case Direction::A_C: return std::string("A_C");
		default:
		case Direction::Changing: return std::string("Changing");
		}
	}

	Turnout::Turnout(const std::string name, const Id index, const Callback callback, const bool leftTurnout)
		: Track(name, index, 0), Shared_Ptr<Turnout>(), callback(callback), trackLengthCalculator(nullptr), leftTurnout(leftTurnout), dir(Direction::A_B), a(), b(), c(), lockingRoutes()
#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
		, _groups()
#endif
	{

	}

	Turnout::Turnout(const std::string name, const Id index, const Callback callback, const TrackLengthCalculator trackLengthCalculator, const bool leftTurnout)
		: Track(name, index, 0), Shared_Ptr<Turnout>(), callback(callback), trackLengthCalculator(trackLengthCalculator), leftTurnout(leftTurnout), dir(Direction::A_B), a(), b(), c(), lockingRoutes()
#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
		, _groups()
#endif
	{

	}

	bool Turnout::has(const Connection connection) const
	{
		return connection != Connection::D && connection != Connection::DeadEnd;
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

	const bool Turnout::traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const
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
			default:
				return false;
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

	const bool Turnout::canTraverse(const Connection entering) const
	{
		return (entering == Connection::A && this->dir != Direction::Changing) ||
			(entering == Connection::B && this->dir == Direction::A_B) ||
			(entering == Connection::C && this->dir == Direction::A_C);
	}

	void Turnout::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
		tracks.insert(b);
		tracks.insert(c);
	}
	/*
	const Track::Connection Turnout::whereConnects(const Track::Shared& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else if (c == other)
			return Track::Connection::C;
		else
			return Track::Connection::DeadEnd;
	}*/

	const Track::Connection Turnout::whereConnects(const Track::Const& other) const
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
		return this->otherConnection(connection, this->dir);
	}

	const Track::Connection Turnout::otherConnection(const Connection connection, const Direction direction) const
	{
		if (direction == Direction::Changing)
			return Connection::DeadEnd;

		if (connection == Connection::A)
			return direction == Direction::A_B ? Connection::B : Connection::C;
		else
			return Connection::A;
	}

	void Turnout::eachConnection(ConnectionCallback callback)
	{
		callback(*this, Connection::A);
		callback(*this, Connection::B);
		callback(*this, Connection::C);
	}

	Track::Shared Turnout::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::A)
		{
			if (a)
				error("Turnout::connect on " + this->name() + ", A already connected");
			a = to;
		}
		else if (local == Connection::B)
		{
			if (b)
				error("Turnout::connect on " + this->name() + ", B already connected");
			b = to;
		}
		else
		{
			if (c)
				error("Turnout::connect on " + this->name() + ", C already connected");
			c = to;
		}
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
		return (this->validateSingle(a) == Result::OK && this->validateSingle(b) == Result::OK && this->validateSingle(c) == Result::OK) ? Result::OK : Result::ValidationFailed;
	}

	const Track::Type Turnout::type() const
	{
		return Type::Turnout;
	}

	void Turnout::connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC)
	{
		onA = a;
		onB = b;
		onC = c;
	}

	const State Turnout::startChangeTo(const Direction direction)
	{
		if (this->locked())
			return State::Finished;

		if (this->dir == direction || this->dir == Direction::Changing)
			return State::Finished;

		this->dir = Direction::Changing;
		return this->callback(*this, this->dir);
	}

	const State Turnout::startToggle()
	{
		return this->startChangeTo(Turnout::otherDirection(this->dir));
	}

	const State Turnout::finalizeChangeTo(const Direction direction)
	{
		if (this->dir == direction)
			return State::Finished;

		State state = this->callback(*this, direction);
		this->dir = direction;

		return state;
	}

	void Turnout::lock(const int route)
	{
		this->lockingRoutes.insert(route);
		this->callback(*this, this->dir);
	}

	void Turnout::unlock(const int route)
	{
		this->lockingRoutes.erase(route);
		if (!this->locked())
			this->callback(*this, this->dir);
	}

	bool Turnout::locked() const
	{
		return this->lockingRoutes.size() > 0;
	}

	const Turnout::Direction Turnout::direction() const
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

	const Track::Connection Turnout::fromDirection() const
	{
		return this->dir == Direction::Changing ? Connection::DeadEnd : (this->dir == Direction::A_B ? Connection::B : Connection::C);
	}

	const Length Turnout::length() const
	{
		return this->lengthOnDirection(this->dir);
	}

	const Length Turnout::lengthOnDirection(const Direction dir) const
	{
		return this->trackLengthCalculator ? this->trackLengthCalculator(dir) : 0;
	}
#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
	void Turnout::addGroup(const Id group, GroupDirection dir)
	{
		this->_groups.emplace(group, dir);
	}

	const bool Turnout::isInGroup(const Id group) const
	{
		return this->_groups.find(group) != this->_groups.end();
	}

	const bool Turnout::isInGroup(const std::map<Id, Turnout::GroupDirection>& groups) const
	{
		for (auto g : groups)
			if (this->isInGroup(g.first))
				return true;

		return false;
	}

	const std::map<Id, Turnout::GroupDirection> &Turnout::groups() const
	{
		return this->_groups;
	}
#endif 

	const size_t Turnout::nextSignalsIndex(const Connection connection, const Signal::Pass pass) const
	{
		switch (connection)
		{
		case Connection::A:
			return pass == Signal::Pass::Facing ? 0 : 1;
		case Connection::B:
			return pass == Signal::Pass::Facing ? 2 : 3;
		case Connection::C:
			return pass == Signal::Pass::Facing ? 4 : 5;
		default:
			return 0;
		}
	}

	void Turnout::setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider)
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			this->nextSignals[idx] = nextSignalProvider;
	}

	Track::NextSignalProvider::Shared Turnout::getNextSignalProvider(const Connection connection, const Signal::Pass pass) const
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			return this->nextSignals[idx];

		return nullptr;
	}

	const std::string DoubleSlipTurnout::DirectionToString(const Direction direction)
	{
		switch (direction)
		{
		case Direction::A_B: return std::string("A_B");
		case Direction::A_C: return std::string("A_C");
		case Direction::B_D: return std::string("B_D");
		case Direction::C_D: return std::string("C_D");
		default:
		case Direction::Changing: return std::string("Changing");
		}
	}

	DoubleSlipTurnout::DoubleSlipTurnout(const std::string name, const Id index, const Callback callback)
		: Track(name, index, 0), Shared_Ptr<DoubleSlipTurnout>(), callback(callback), trackLengthCalculator(nullptr), dir(Direction::A_B), accessoryStates{0xF0, 0x0D}, a(), b(), c(), d(), lockingRoutes()
	{

	}

	DoubleSlipTurnout::DoubleSlipTurnout(const std::string name, const Id index, const Callback callback, const TrackLengthCalculator trackLengthCalculator)
		: Track(name, index, 0), Shared_Ptr<DoubleSlipTurnout>(), callback(callback), trackLengthCalculator(trackLengthCalculator), dir(Direction::A_B), accessoryStates{0xF0, 0x0D}, a(), b(), c(), d(), lockingRoutes()
	{

	}

	bool DoubleSlipTurnout::has(const Connection connection) const
	{
		return connection != Connection::DeadEnd;
	}

	Track::Shared DoubleSlipTurnout::on(const Connection connection) const
	{
		if (connection == Connection::A)
			return a;
		else if (connection == Connection::B)
			return b;
		else if (connection == Connection::C)
			return c;
		else if (connection == Connection::D)
			return d;
		return nullptr;
	}

	const bool DoubleSlipTurnout::traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const
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
			case Connection::D: onto = d; return true; break;
			default:
				return false;
			}
		}
		else
		{
			if (connection == Connection::A)
			{
				if (this->dir == Direction::A_B) onto = b;
				else if (this->dir == Direction::A_C) onto = c;
				return true;
			}
			else if (connection == Connection::B)
			{
				if (this->dir == Direction::A_B) onto = a;
				else if (this->dir == Direction::B_D) onto = d;
				return true;
			}
			else if (connection == Connection::C)
			{
				if (this->dir == Direction::A_C) onto = a;
				else if (this->dir == Direction::C_D) onto = d;
				return true;
			}
			else if (connection == Connection::D)
			{
				if (this->dir == Direction::C_D) onto = c;
				else if (this->dir == Direction::B_D) onto = d;
				return true;
			}
		}
		return false;
	}

	const bool DoubleSlipTurnout::canTraverse(const Connection entering) const
	{
		return (entering == Connection::A && (this->dir == Direction::A_B || this->dir == Direction::A_C)) ||
			(entering == Connection::B && (this->dir == Direction::A_B || this->dir == Direction::B_D)) ||
			(entering == Connection::C && (this->dir == Direction::A_C || this->dir == Direction::C_D)) ||
			(entering == Connection::D && (this->dir == Direction::B_D || this->dir == Direction::C_D));
	}

	void DoubleSlipTurnout::collectAllConnections(std::set<Track::Shared>& tracks) const
	{
		tracks.insert(a);
		tracks.insert(b);
		tracks.insert(c);
		tracks.insert(d);
	}
	/*
	const Track::Connection DoubleSlipTurnout::whereConnects(const Track::Shared& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else if (c == other)
			return Track::Connection::C;
		else if (d == other)
			return Track::Connection::D;
		else
			return Track::Connection::DeadEnd;
	}*/

	const Track::Connection DoubleSlipTurnout::whereConnects(const Track::Const& other) const
	{
		if (a == other)
			return Track::Connection::A;
		else if (b == other)
			return Track::Connection::B;
		else if (c == other)
			return Track::Connection::C;
		else if (d == other)
			return Track::Connection::D;
		else
			return Track::Connection::DeadEnd;
	}

	const Track::Connection DoubleSlipTurnout::otherConnection(const Connection connection) const
	{
		return this->otherConnection(connection, this->dir);
	}

	const Track::Connection DoubleSlipTurnout::otherConnection(const Connection connection, const Direction direction) const
	{
		if (this->dir == Direction::Changing)
			return Connection::DeadEnd;

		if (connection == Connection::A)
		{
			if (this->dir == Direction::A_B) return Connection::B;
			else if (this->dir == Direction::A_C) return Connection::C;
			else return Connection::DeadEnd;
		}
		else if (connection == Connection::B)
		{
			if (this->dir == Direction::A_B) return Connection::A;
			else if (this->dir == Direction::B_D) return Connection::D;
			else return Connection::DeadEnd;
		}
		else if (connection == Connection::C)
		{
			if (this->dir == Direction::A_C) return Connection::A;
			else if (this->dir == Direction::C_D) return Connection::D;
			else return Connection::DeadEnd;
		}
		else if (connection == Connection::D)
		{
			if (this->dir == Direction::B_D) return Connection::B;
			else if (this->dir == Direction::C_D) return Connection::C;
			else return Connection::DeadEnd;
		}
		else
			return Connection::DeadEnd;
	}

	void DoubleSlipTurnout::eachConnection(ConnectionCallback callback)
	{
		callback(*this, Connection::A);
		callback(*this, Connection::B);
		callback(*this, Connection::C);
		callback(*this, Connection::D);
	}

	Track::Shared DoubleSlipTurnout::connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa)
	{
		if (local == Connection::A)
		{
			if (a)
				error("Turnout::connect on " + this->name() + ", A already connected");
			a = to;
		}
		else if (local == Connection::B)
		{
			if (b)
				error("Turnout::connect on " + this->name() + ", B already connected");
			b = to;
		}
		else if(local == Connection::C)
		{
			if (c)
				error("Turnout::connect on " + this->name() + ", C already connected");
			c = to;
		}
		else
		{
			if (d)
				error("Turnout::connect on " + this->name() + ", D already connected");
			d = to;
		}
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

	const Result DoubleSlipTurnout::validate()
	{
		return (this->validateSingle(a) == Result::OK 
			&& this->validateSingle(b) == Result::OK 
			&& this->validateSingle(c) == Result::OK 
			&& this->validateSingle(d) == Result::OK) ? Result::OK : Result::ValidationFailed;
	}

	const Track::Type DoubleSlipTurnout::type() const
	{
		return Type::DoubleSlipTurnout;
	}

	void DoubleSlipTurnout::connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC, Track::Shared& onD)
	{
		onA = a;
		onB = b;
		onC = c;
		onD = d;
	}

	const State DoubleSlipTurnout::startChangeTo(const Direction direction)
	{
		if (this->locked())
			return State::Finished;

		if (this->dir == direction || this->dir == Direction::Changing)
			return State::Finished;

		this->dir = Direction::Changing;
		return this->callback(*this, this->dir);
	}

	const State DoubleSlipTurnout::startToggle()
	{
		return this->startChangeTo(DoubleSlipTurnout::nextDirection(this->dir));
	}

	const State DoubleSlipTurnout::finalizeChangeTo(const Direction direction)
	{
		if (this->dir == direction)
			return State::Finished;

		State state = this->callback(*this, direction);
		this->dir = direction;

		return state;
	}

	void DoubleSlipTurnout::lock(const int route)
	{
		this->lockingRoutes.insert(route);
		this->callback(*this, this->dir);
	}

	void DoubleSlipTurnout::unlock(const int route)
	{
		this->lockingRoutes.erase(route);
		if(!this->locked())
			this->callback(*this, this->dir);
	}

	bool DoubleSlipTurnout::locked() const
	{
		return this->lockingRoutes.size() > 0;
	}

	void DoubleSlipTurnout::setAccessoryState(const unsigned char state, const bool first, const bool applyToInternalDirection, const bool doCallback)
	{
		this->accessoryStates[first ? 0 : 1] = state;

		if (applyToInternalDirection && this->isKnownAccessoryState())
		{
			this->dir = this->fromAccessoryState();
			if (doCallback)
				this->callback(*this, this->dir);
		}
	}

	const bool DoubleSlipTurnout::isKnownAccessoryState() const
	{
		return (this->accessoryStates[0] == 0 || this->accessoryStates[0] == 1) &&
			(this->accessoryStates[1] == 0 || this->accessoryStates[1] == 1);
	}

	/*
	*          Z1=D
	*           \\   
	* Z2=A ==== Turnout8_9 ==== Z3=C
	*             \\
	*              Z4=B
	* Z1 -> Z3: CD
	* Z1 -> Z4: BD
	* Z4 -> Z2: AB
	* Z2 -> A3: AC
	*/
	const void DoubleSlipTurnout::toAccessoryStates(unsigned char& a, unsigned char& b, const Direction direction) const
	{
		switch (direction)
		{
		case Direction::A_B: a = 0; b = 1; break;
		case Direction::A_C: a = 1; b = 1; break;
		case Direction::B_D: a = 0; b = 0; break;
		case Direction::C_D: a = 1; b = 0; break;
		default:
		case Direction::Changing:
			break;
		}
	}

	const DoubleSlipTurnout::Direction DoubleSlipTurnout::fromAccessoryState() const
	{
		if (!this->isKnownAccessoryState())
			return Direction::Changing;
		else if (this->accessoryStates[0] && this->accessoryStates[1])
			return Direction::A_C;
		else if (!this->accessoryStates[0] && this->accessoryStates[1])
			return Direction::A_B;
		else if (this->accessoryStates[0] && !this->accessoryStates[1])
			return Direction::C_D;
		else if (!this->accessoryStates[0] && !this->accessoryStates[1])
			return Direction::B_D;
		return Direction::Changing;
	}
	/*
	const DoubleSlipTurnout::Direction DoubleSlipTurnout::fromAccessoryState(const unsigned char state, const bool first)
	{
		unsigned char a = 99, b = 99;
		//this->toAccessoryStates(a, b);

		//if (a == 99 || b == 99)
		//	return Direction::Changing;

		if (first)
			a = state ? 1 : 0;
		else
			b = state ? 1 : 0;

		this->accessoryStates[first ? 0 : 1] = state;

		if (a && b) 
			return Direction::A_B;
		else if (!a && b) 
			return Direction::A_C;
		else if (a && !b) 
			return Direction::B_D;
		else if (!a && !b) 
			return Direction::C_D;
		else 
			return Direction::Changing;
	}
	*/
	const DoubleSlipTurnout::Direction DoubleSlipTurnout::direction() const
	{
		return this->dir;
	}

	const DoubleSlipTurnout::Direction DoubleSlipTurnout::nextDirection(const Direction current)
	{
		switch (current)
		{
		case Direction::A_B: return Direction::A_C;
		case Direction::A_C: return Direction::C_D;
		case Direction::C_D: return Direction::B_D;
		case Direction::B_D: return Direction::A_B;
		default:
			return Direction::Changing;
		}
	}

	const Track::Connection DoubleSlipTurnout::fromDirection() const
	{
		switch (this->dir)
		{
		case Direction::A_B:
			return Connection::A;
		case Direction::A_C:
			return Connection::C;
		case Direction::B_D:
			return Connection::B;
		case Direction::C_D:
			return Connection::D;
		case Direction::Changing:
		default:
			return Connection::DeadEnd;
		}
	}

	const Length DoubleSlipTurnout::length() const
	{
		return this->lengthOnDirection(this->dir);
	}

	const Length DoubleSlipTurnout::lengthOnDirection(const Direction dir) const
	{
		return this->trackLengthCalculator ? this->trackLengthCalculator(dir) : 0;
	}

	const size_t DoubleSlipTurnout::nextSignalsIndex(const Connection connection, const Signal::Pass pass) const 
	{
		switch (connection)
		{
		case Connection::A:
			return pass == Signal::Pass::Facing ? 0 : 1;
		case Connection::B:
			return pass == Signal::Pass::Facing ? 2 : 3;
		case Connection::C:
			return pass == Signal::Pass::Facing ? 4 : 5;
		case Connection::D:
			return pass == Signal::Pass::Facing ? 6 : 7;
		default:
			return 0;
		}

	}

	void DoubleSlipTurnout::setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider)
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			this->nextSignals[idx] = nextSignalProvider;
	}

	Track::NextSignalProvider::Shared DoubleSlipTurnout::getNextSignalProvider(const Connection connection, const Signal::Pass pass) const
	{
		const auto idx = this->nextSignalsIndex(connection, pass);
		if (idx < this->nextSignals.size())
			return this->nextSignals[idx];

		return nullptr;
	}
}