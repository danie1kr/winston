#ifndef WINSTON_TRACK_H
#define WINSTON_TRACK_H
#pragma once

#include <array>
#include <vector>
#include <set>
#include <map>
#include <unordered_set>
#include <memory>
#include <functional>
#include <variant>
#include "Signal.h"
#include "WinstonTypes.h"

namespace winston
{
	/*
	        =======
	     S4/ S5  S3
	======/========
	  S1        S2
	*/
	class Track : public Shared_Ptr<Track>
	{
	public:

		Track(const std::string name, const Id index, const Length tracklength);
		~Track() = default;

		enum class Connection : uint8_t
		{
			A, B, C, D, DeadEnd
		};

		static const std::string ConnectionToString(const Connection connection);
		static const Connection ConnectionFromString(const std::string connection);

		enum class Type
		{
			Bumper,
			Rail,
			Turnout,
			DoubleSlipTurnout
		};

		using SignalFactory = std::function<Signal::Shared(Track::Shared track, Track::Connection connection)>;

		Track::Shared connect(const Connection local, Track::Shared& to, const Connection remote);
		virtual bool has(const Connection connection) const = 0;
		virtual Track::Shared on(const Connection connection) const = 0;
		
		void section(const Address address);
		const Address section() const;

		virtual const bool traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const = 0;
		virtual const bool canTraverse(const Connection entering) const = 0;

		using TraversalCallback = std::function<const Result(Track::Const track, const Track::Connection connection)>;
		enum class TraversalResult : unsigned int
		{
			Looped,
			Bumper,
			OpenTurnout,
			Signal,
			CancelledByCallback
		}; enum class TraversalSignalHandling : unsigned int
		{
			Ignore,
			ForwardDirection,
			OppositeDirection
		};
		template<TraversalSignalHandling _signalHandling, bool includingFirst = true >
		static TraversalResult traverse(Track::Const& start, Track::Connection& connection, Signal::Shared& signal, TraversalCallback callback = nullptr)
		{
			auto& current = start;
			auto fixedStart = start;
			auto onto = current;
			std::unordered_set<Track::Const> visited;
			bool successful = true;
			bool checkForward = includingFirst;
			while (true)
			{
				if (checkForward && _signalHandling == TraversalSignalHandling::ForwardDirection)
				{// the signal faces us
					auto facingConnection = onto->otherConnection(connection);
					signal = onto->signalFacing(facingConnection);
					if (signal)
					{
						start = onto;
						connection = facingConnection;
						return TraversalResult::Signal;
					}
				}
				checkForward = true;
				if (visited.find(onto) != visited.end())
					return TraversalResult::Looped;
				visited.insert(onto);
				successful = current->traverse(connection, onto, true);
				if (!successful)
				{
					if (current->type() == Track::Type::Turnout || current->type() == Track::Type::DoubleSlipTurnout)
					{
						start = current;
						connection = connection;
						return TraversalResult::OpenTurnout;
					}
					else if (current->type() == Track::Type::Bumper)
					{
						return TraversalResult::Bumper;
					}
				}
				if (callback)
					if (callback(onto, connection) != Result::OK)
						return TraversalResult::CancelledByCallback;
				connection = onto->whereConnects(current);
				if((onto->type() == Track::Type::Turnout || onto->type() == Track::Type::DoubleSlipTurnout) && !onto->canTraverse(connection))
				{
					start = current;
					connection = connection;
					return TraversalResult::OpenTurnout;
				}
				if (_signalHandling == TraversalSignalHandling::OppositeDirection)
				{// the signal looks in the same way as we travel
					signal = onto->signalGuarding(connection);
					if (signal)
					{
						start = onto;
						connection = connection;
						return TraversalResult::Signal;
					}
				}
				connection = onto->otherConnection(connection);
				if (connection == Connection::DeadEnd)
					return TraversalResult::Bumper;
				if (callback)
					if (callback(onto, connection) != Result::OK)
						return TraversalResult::CancelledByCallback;
				current = onto;
			}
		}

		static TraversalResult traverse(Track::Const& start, Track::Connection& connection)
		{
			Signal::Shared signal;
			return traverse<TraversalSignalHandling::Ignore>(start, connection, signal);
		}

		const bool traverseToNextSegment(const Track::Connection connection, const bool leavingOnConnection, Track::Const& onto, Track::Connection& ontoConnection) const;

		virtual void collectAllConnections(std::set<Track::Shared>& tracks) const = 0;
		//virtual const Connection whereConnects(const Track::Shared& other) const = 0;
		virtual const Connection whereConnects(const Track::Const& other) const = 0;
		virtual const Connection otherConnection(const Connection connection) const = 0;
		using ConnectionCallback = std::function<void(Track& track, const Connection connection)>;
		virtual void eachConnection(ConnectionCallback callback) = 0;

		virtual const Result validate() = 0;
		virtual const Type type() const = 0;

		virtual void attachSignal(Signal::Shared signal, const Connection guarding);
		virtual Signal::Shared signalFacing(const Connection facing) const;
		virtual Signal::Shared signalGuarding(const Connection guarding) const;
		virtual const Length length() const;
		const std::string name() const;

		const Id index;

		const Id segment() const;
		void segment(const Id segment);
		
		using Shared_Ptr<Track>::make;
		using Shared_Ptr<Track>::Const;
		using Shared_Ptr<Track>::Shared;

		struct NextSignalProvider : public Shared_Ptr<NextSignalProvider>
		{
			struct NextTurnout : public Shared_Ptr<NextTurnout>
			{
				NextTurnout(const Track::Const turnout, const Track::Connection entered);
				~NextTurnout() = default;

				const Track::Const turnout;
				const Track::Connection entered;
			};

			NextSignalProvider(const Distance distance, const Signal::Shared signal, const Track::Const track, const Track::Connection connection);
			NextSignalProvider(const Distance distance, const NextTurnout::Const nextTurnout);
			~NextSignalProvider() = default;

			const Distance distance;
			const Signal::Shared signal;
			const Track::Const track;
			const Track::Connection connection;
			const NextTurnout::Const nextTurnout;
		};
		virtual void setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider) = 0;
		//const NextSignal::Shared nextSignal(const Connection leaving, const Signal::Pass pass) const;
		const bool nextSignal(const Connection leaving, const Signal::Pass pass, Signal::Shared& signal, Track::Const& track, Track::Connection& guarding, Distance &distance) const;
		virtual const size_t nextSignalsIndex(const Connection connection, const Signal::Pass pass) const = 0;

		virtual NextSignalProvider::Shared getNextSignalProvider(const Connection connection, const Signal::Pass pass) const = 0;

	protected:
		virtual Track::Shared connectTo(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true) = 0;
		const Result validateSingle(const Track::Shared track) const;

		friend class Bumper; 
		friend class Rail;
		friend class Turnout;
		friend class DoubleSlipTurnout;

	private:
		const std::string _name;
		Id _segment;
		Address _section;
		const Length trackLength;
	};

	using TrackSet = std::set<Track::Shared>;
	using TrackList = std::list<Track::Shared>;
	
	// a====|
	class Bumper : public Track, public Shared_Ptr<Bumper>
	{
	public:
		using Shared_Ptr<Bumper>::Shared;
		using Shared_Ptr<Bumper>::make;
		using Shared_Ptr<Bumper>::enable_shared_from_this_virtual::shared_from_this;

		Bumper(const std::string name = "", const Id index = 0xBADF00D, const Length tracklength = 0);
		~Bumper() = default;

		bool has(const Connection connection) const;
		Track::Shared on(const Connection connection) const;
		const bool traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const;
		const bool canTraverse(const Connection entering) const;
		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		//const Connection whereConnects(const Track::Shared& other) const;
		const Connection whereConnects(const Track::Const& other) const;
		const Connection otherConnection(const Connection connection) const;
		void eachConnection(ConnectionCallback callback);

		const Result validate();
		const Type type() const;

		void attachSignal(Signal::Shared signal, const Connection guarding);
		Signal::Shared signalFacing(const Connection facing) const;
		Signal::Shared signalGuarding(const Connection guarding) const;

		void connections(Track::Shared& onA);

		void setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider);
#ifdef WINSTON_TEST
	public:
#else
	private:
#endif
		NextSignalProvider::Shared getNextSignalProvider(const Connection connection, const Signal::Pass pass) const;
	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		Track::Shared a;
		std::array<Signal::Shared, 2> signals;

		const size_t nextSignalsIndex(const Connection connection, const Signal::Pass pass) const;
		std::array<NextSignalProvider::Shared, 3> nextSignals;
	};

	// a====b
	class Rail : public Track, public Shared_Ptr<Rail>
	{
	public:
		using Shared_Ptr<Rail>::Shared;
		using Shared_Ptr<Rail>::make;
		using Shared_Ptr<Rail>::enable_shared_from_this_virtual::shared_from_this;

		Rail(const std::string name = "", const Id index = 0xBADF00D, const Length tracklength = 0);
		~Rail() = default;

		bool has(const Connection connection) const;
		Track::Shared on(const Connection connection) const;
		const bool traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const;
		const bool canTraverse(const Connection entering) const;

		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		//const Connection whereConnects(const Track::Shared& other) const;
		const Connection whereConnects(const Track::Const& other) const;
		const Connection otherConnection(const Connection connection) const;
		void eachConnection(ConnectionCallback callback);

		const Result validate();
		const Type type() const;

		void attachSignal(Signal::Shared signal, const Connection guarding);
		Signal::Shared signalFacing(const Connection facing) const;
		Signal::Shared signalGuarding(const Connection guarding) const;

		void connections(Track::Shared& onA, Track::Shared& onB);

		void setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider);
#ifdef WINSTON_TEST
	public:
#else
	private:
#endif
		NextSignalProvider::Shared getNextSignalProvider(const Connection connection, const Signal::Pass pass) const;
	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		Track::Shared a, b;
		std::array<Signal::Shared, 2> signals;

		const size_t nextSignalsIndex(const Connection connection, const Signal::Pass pass) const;
		std::array<NextSignalProvider::Shared, 4> nextSignals;
	};

	// a====b
	//   \==c
	class Turnout : public Track, public Shared_Ptr<Turnout>
	{
	public:
		using Shared_Ptr<Turnout>::Shared;
		using Shared_Ptr<Turnout>::make;
		using Shared_Ptr<Turnout>::enable_shared_from_this_virtual::shared_from_this;

		enum class Direction : unsigned int
		{
			A_B = 0,
			A_C = 1,
			Changing = 8
		};
		static const std::string DirectionToString(const Direction direction);

		using Callback = const std::function<State(Track& turnout, Direction direction)>;

		using TrackLengthCalculator = const std::function<const Length(const Direction)>;

		Turnout(const std::string name, const Id index, const Callback callback, const bool leftTurnout = false);
		Turnout(const std::string name, const Id index, const Callback callback, const TrackLengthCalculator trackLengthCalculator, const bool leftTurnout = false);
		~Turnout() = default;

		bool has(const Connection connection) const;
		Track::Shared on(const Connection connection) const;

		const bool traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const;
		const bool canTraverse(const Connection entering) const;
		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		//const Connection whereConnects(const Track::Shared& other) const;
		const Connection whereConnects(const Track::Const& other) const;
		const Connection otherConnection(const Connection connection) const;
		const Connection otherConnection(const Connection connection, const Direction direction) const;
		void eachConnection(ConnectionCallback callback);

		const Result validate();
		const Type type() const;

		void connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC);

		void setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider);
#ifdef WINSTON_TEST
	public:
#else
	private:
#endif
		NextSignalProvider::Shared getNextSignalProvider(const Connection connection, const Signal::Pass pass) const;
	public:
		const State startChangeTo(const Direction direction);
		const State startToggle();
		const State finalizeChangeTo(const Direction direction);

		void lock(const int route);
		void unlock(const int route);
		bool locked() const;

#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
		enum class GroupDirection : unsigned char
		{
			Unknown,
			Same,
			Opposite
		};
		void addGroup(const Id group, const GroupDirection dir);
		const bool isInGroup(const Id group) const;
		const bool isInGroup(const std::map<Id, Turnout::GroupDirection>& groups) const;
		const std::map<Id, Turnout::GroupDirection> &groups() const;

#endif
		const Direction direction() const;
		static const Direction otherDirection(const Direction current);
		const Connection fromDirection() const;

		virtual const Length length() const;
		const Length lengthOnDirection(const Direction dir) const;
	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		Callback callback;
		const TrackLengthCalculator trackLengthCalculator;

		bool leftTurnout;
		Direction dir;

		Track::Shared a, b, c;

		std::unordered_set<unsigned int> lockingRoutes;
#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
		std::map<Id, GroupDirection> _groups;
#endif 

		const size_t nextSignalsIndex(const Connection connection, const Signal::Pass pass) const;
		std::array<NextSignalProvider::Shared, 6> nextSignals;
	};
	using TurnoutSet = std::unordered_set<Turnout::Shared>;

	/* A   D
	    \ /
	     X
	    / \
	   B   C */
	class DoubleSlipTurnout : public Track, public Shared_Ptr<DoubleSlipTurnout>
	{
	public:
		using Shared_Ptr<DoubleSlipTurnout>::Shared;
		using Shared_Ptr<DoubleSlipTurnout>::make;
		using Shared_Ptr<DoubleSlipTurnout>::enable_shared_from_this_virtual::shared_from_this;

		enum class Direction : unsigned int
		{
			A_B = 2,
			A_C = 3,
			C_D = 4,
			B_D = 5,
			Changing = 8
		};

		static const std::string DirectionToString(const Direction direction);

		using Callback = const std::function<State(Track& turnout, Direction direction)>;

		using TrackLengthCalculator = const std::function<const Length(const Direction)>;

		DoubleSlipTurnout(const std::string name, const Id index, const Callback callback);
		DoubleSlipTurnout(const std::string name, const Id index, const Callback callback, const TrackLengthCalculator trackLengthCalculator);
		~DoubleSlipTurnout() = default;

		bool has(const Connection connection) const;
		Track::Shared on(const Connection connection) const;

		const bool traverse(const Connection connection, Track::Const& onto, const bool leavingOnConnection) const;
		const bool canTraverse(const Connection entering) const;
		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		//const Connection whereConnects(const Track::Shared& other) const;
		const Connection whereConnects(const Track::Const& other) const;
		const Connection otherConnection(const Connection connection) const;
		const Connection otherConnection(const Connection connection, const Direction direction) const;
		void eachConnection(ConnectionCallback callback);

		const Result validate();
		const Type type() const;

		void connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC, Track::Shared& onD);

		void setupNextSignal(const Connection connection, const Signal::Pass pass, const NextSignalProvider::Shared nextSignalProvider);
#ifdef WINSTON_TEST
	public:
#else
	private:
#endif
		NextSignalProvider::Shared getNextSignalProvider(const Connection connection, const Signal::Pass pass) const;
	public:

		const State startChangeTo(const Direction direction);
		const State startToggle();
		const State finalizeChangeTo(const Direction direction);

		void lock(const int route);
		void unlock(const int route);
		bool locked() const;

		void setAccessoryState(const unsigned char state, const bool first, const bool applyToInternalDirection, const bool doCallback);
		const bool isKnownAccessoryState() const;
		const Direction fromAccessoryState() const;

		const void toAccessoryStates(unsigned char& a, unsigned char& b, const Direction direction) const;
		const Direction direction() const;
		static const Direction nextDirection(const Direction current);
		const Connection fromDirection() const;

		virtual const Length length() const;
		const Length lengthOnDirection(const Direction dir) const;
	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);

		Callback callback;
		const TrackLengthCalculator trackLengthCalculator;

		Direction dir;
		unsigned char accessoryStates[2];

		Track::Shared a, b, c, d;

		std::unordered_set<unsigned int> lockingRoutes;

		const size_t nextSignalsIndex(const Connection connection, const Signal::Pass pass) const;
		std::array<NextSignalProvider::Shared, 8> nextSignals;
	};

	std::string build(const Track::Connection first);
}
#endif