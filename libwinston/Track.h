#pragma once

#include <array>
#include <vector>
#include <set>
#include <unordered_set>
#include <memory>
#include <functional>
#include "Signal.h"
#include "WinstonTypes.h"

namespace winston
{
	/*
	// Block: a collection of rails secured by signals
	class Block
	{
	public:
		Block(const std::vector<Track&> tracks);
		std::vector<Track*> tracks;
		std::array<Signal*, 2> signals;

		void entered(Locomotive& loco, Track& onto);
		void left(Locomotive& loco, Track& to);
	};*/

	/*
	        =======
	     S4/ S5  S3
	======/========
	  S1        S2
	*/
	class Track : public Shared_Ptr<Track>
	{
	public:

		Track(std::string name, Length tracklength = 0);

		enum class Connection : unsigned int
		{
			A, B, C, DeadEnd
		};

		static const std::string ConnectionToString(const Connection connection);
		static const Connection ConnectionFromString(const std::string connection);

		enum class Type
		{
			Bumper,
			Rail,
			Turnout
		};

		using SignalFactory = std::function<Signal::Shared(Track::Shared track, Track::Connection connection)>;

		//virtual bool drive(TrackIndex& enter, TrackIndex& exit, bool forward = true) const = 0;

		Track::Shared connect(const Connection local, Track::Shared& to, const Connection remote);
		Track::Shared connect(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote);
		Track::Shared connect(const Connection local, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory);
		Track::Shared connect(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory);
		virtual bool has(const Connection connection) const = 0;
		//Track& attachSignal(Signal &signal, const Connection comingFrom);

		virtual bool traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const = 0;

		enum class TraversalResult : unsigned int
		{
			Looped,
			Bumper,
			OpenTurnout,
			Signal
		}; enum class TraversalSignalHandling : unsigned int
		{
			Ignore,
			ForwardDirection,
			OppositeDirection
		};
		template<TraversalSignalHandling _signalHandling >
		static TraversalResult traverse(Track::Shared& start, Track::Connection& connection, Signal::Shared& signal)
		{
			auto& current = start;
			Track::Shared onto = current;
			std::unordered_set<Track::Shared> visited;
			bool successful = true;
			while (true)
			{
				if (_signalHandling == TraversalSignalHandling::ForwardDirection)
				{// the signal faces us
					signal = onto->signalFacing(connection);
					if (signal)
					{
						start = onto;
						connection = connection;
						return TraversalResult::Signal;
					}
				}
				if (visited.contains(onto))
					return TraversalResult::Looped;
				if (onto->type() == Track::Type::Bumper)
					return TraversalResult::Bumper;
				visited.insert(onto);
				successful = current->traverse(connection, onto, true);
				if (!successful && current->type() == Track::Type::Turnout)
				{
					start = current;
					connection = connection;
					return TraversalResult::OpenTurnout;
				}

				connection = onto->whereConnects(current);
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
				current = onto;
			}
		}

		virtual void collectAllConnections(std::set<Track::Shared>& tracks) const = 0;
		virtual const Connection whereConnects(Track::Shared& other) const = 0;
		virtual const Connection otherConnection(const Connection connection) const = 0;
		virtual const Result validate() = 0;
		virtual const Type type() = 0;

		virtual void attachSignal(Signal::Shared signal, const Connection guarding);
		virtual Signal::Shared signalFacing(const Connection facing);
		virtual Signal::Shared signalGuarding(const Connection guarding);
		virtual const Length length();
		const std::string name();

	protected:
		virtual Track::Shared connectTo(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true) = 0;
		Result validateSingle(const Track::Shared track);

		friend class Bumper; 
		friend class Rail;
		friend class Turnout;

	private:
		const Length trackLength;
		const std::string _name;
	};
	
	// a====|
	class Bumper : public Track, public Shared_Ptr<Bumper>, public std::enable_shared_from_this<Bumper>
	{
	public:
		Bumper(const std::string name = "");
		//static Track::Shared make();

		bool has(const Connection connection) const;
		bool traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const;
		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		const Connection whereConnects(Track::Shared& other) const;
		const Connection otherConnection(const Connection connection) const;
		const Result validate();
		const Type type();

		void attachSignal(Signal::Shared signal, const Connection guarding);
		Signal::Shared signalFacing(const Connection facing);
		Signal::Shared signalGuarding(const Connection guarding);

		void connections(Track::Shared& onA);

		using Shared_Ptr<Bumper>::Shared;
		using Shared_Ptr<Bumper>::make;

	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		Track::Shared a;
		std::array<Signal::Shared, 2> signals;
	};

	// a====b
	class Rail : public Track, public Shared_Ptr<Rail>, public std::enable_shared_from_this<Rail>
	{
	public:
		Rail(const std::string name = "");
		//static Track::Shared make();

		bool has(const Connection connection) const;
		bool traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const;

		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		const Connection whereConnects(Track::Shared& other) const;
		const Connection otherConnection(const Connection connection) const;
		const Result validate();
		const Type type();

		void attachSignal(Signal::Shared signal, const Connection guarding);
		Signal::Shared signalFacing(const Connection facing);
		Signal::Shared signalGuarding(const Connection guarding);

		void connections(Track::Shared& onA, Track::Shared& onB);

		using Shared_Ptr<Rail>::Shared;
		using Shared_Ptr<Rail>::make;
	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		Track::Shared a, b;
		std::array<Signal::Shared, 2> signals;
	};

	// a====b
	//   \==c
	class Turnout : public Track, public Shared_Ptr<Turnout>, public std::enable_shared_from_this<Turnout>
	{
	public:
		enum class Direction : unsigned int
		{
			A_B,
			A_C,
			Changing
		};

		using Callback = const std::function<State(Track::Shared turnout, Direction direction)>;

		using TrackLengthCalculator = const std::function<const Length(const Direction)>;

		//Turnout(const Callback callback, const bool leftTurnout = false);
		Turnout(const std::string name, const Callback callback, const bool leftTurnout = false);
		Turnout(const std::string name, const Callback callback, const TrackLengthCalculator trackLengthCalculator, const bool leftTurnout = false);
		//static Track::Shared make(const Callback callback, const bool leftTurnout);

		bool has(const Connection connection) const;

		bool traverse(const Connection connection, Track::Shared& onto, bool leavingOnConnection) const;
		void collectAllConnections(std::set<Track::Shared>& tracks) const;
		const Connection whereConnects(Track::Shared& other) const;
		const Connection otherConnection(const Connection connection) const;

		const Result validate();
		const Type type();

		void connections(Track::Shared& onA, Track::Shared& onB, Track::Shared& onC);

		//const Task::State toggle();
		const State startChangeTo(const Direction direction);
		const State startToggle();
		const State finalizeChangeTo(const Direction direction);

		const Direction direction();
		static const Direction otherDirection(const Direction current);
		static const Direction fromConnection(const Connection connection);
		void fromDirection(Track::Shared& a, Track::Shared& other) const;
		const Connection fromDirection() const;

		using Shared_Ptr<Turnout>::Shared;
		using Shared_Ptr<Turnout>::make;
		virtual const Length length();

	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		const TrackLengthCalculator trackLengthCalculator;

		bool leftTurnout;
		Direction dir;

		Callback callback;
		Track::Shared a, b, c;
	};
}