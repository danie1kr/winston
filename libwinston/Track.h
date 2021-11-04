#pragma once

#include <array>
#include <vector>
#include <set>
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

		Track();

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
		virtual void collectAllConnections(std::set<Track::Shared>& tracks) const = 0;
		virtual const Connection whereConnects(Track::Shared& other) const = 0;
		virtual const Connection otherConnection(const Connection connection) const = 0;
		virtual const Result validate() = 0;
		virtual const Type type() = 0;

		virtual void attachSignal(Signal::Shared signal, const Connection guarding);
		virtual Signal::Shared signalFacing(const Connection facing);
		virtual Signal::Shared signalGuarding(const Connection guarding);

	protected:
		virtual Track::Shared connectTo(const Connection local, SignalFactory guardingLocalSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true) = 0;
		Result validateSingle(const Track::Shared track);

		friend class Bumper; 
		friend class Rail;
		friend class Turnout;
	};
	
	// a====|
	class Bumper : public Track, public Shared_Ptr<Bumper>, public std::enable_shared_from_this<Bumper>
	{
	public:
		Bumper();
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
		Rail();
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
		enum class Direction
		{
			A_B,
			A_C,
			Changing
		};

		using Callback = const std::function<State(Track::Shared turnout, Direction direction)>;

		Turnout(const Callback callback, const bool leftTurnout = false);
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
		static Direction otherDirection(const Direction current);

		using Shared_Ptr<Turnout>::Shared;
		using Shared_Ptr<Turnout>::make;

	private:
		Track::Shared connectTo(const Connection local, SignalFactory guardingSignalFactory, Track::Shared& to, const Connection remote, SignalFactory guardingRemoteSignalFactory, bool viceVersa = true);
		
		bool leftTurnout;
		Direction dir;

		Callback callback;
		Track::Shared a, b, c;
	};
}