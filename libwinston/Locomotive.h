#pragma once

#include <map>
#include "WinstonTypes.h"
#include "Position.h"
#include "Track.h"
#include "NextSignals.h"
#include "Log.h"
#include "Segment.h"
#include "HAL.h"

namespace winston
{
	/*
	    v1        /------
		         /
		v0 -----/     
		               
	        |   | |    |  
			t0  tatb   t1
			 T0  Ta  T1

		s = v*t
		v = a*t
		s = 1/2 a * t*t

		known: v0, v1, s, t0, ta, t1, T0

		tb = 

		s = v0 * (ta-t0) + a/2 * (tb-ta)� + v1 * (t1-tb)
		s = v0 * (T0) + a/2 * (Ta)� + v1 * (T1)
		===================================================
		v1    /------
			 /
		v0 -/
		 0
			| |    |
			t0ta   t1

		known: v0, v1, s, t0, t1
		a = (v1 - v0) / (ta - t0)
		s = v0 * (ta-t0) + a/2 * (ta-t0)� + v1 * (t1-ta)
		s = v0 * (ta-t0) + 1/2 * (v1 - v0) / (ta - t0) * (ta-t0)� + v1 * (t1-ta)
		s = v0 * (ta-t0) + 1/2 * (v1 - v0) * (ta-t0) + v1 * (t1-ta)
		s = (ta-t0) * (v0 + (v1 - v0)/2) + v1 * (t1 - ta)

		s / (v0 + (v1 - v0)/2) = ta + v1*t1 - v1*ta
		s / (v0 + (v1 - v0)/2) - v1*t1 = ta * (1 - v1)
		(s / (v0 + (v1 - v0)/2) - v1*t1) / (1 - v1) = ta

		solve s = (v0 * (ta-t0)) + (1/2 * (v1 - v0) * (ta-t0)) + (v1 * (t1-ta) ) for ta
		t = (2 s + t0 (v0 + v1) - 2 t1 v1)/(v0 - v1) and v0!=v1

		t0 = 0:	
		solve s = (v0 * (t)) + (1/2 * (v1 - v0) * (t)) + (v1 * (t1-t) ) for t
		t = (2 (s - t1 v1))/(v0 - v1) and v0!=v1
		a = (v1 - v0) / t

	*/
	// https://www.wolframalpha.com/input/?i=solve+s+%3D+%28v1%C2%B2-v0%C2%B2%29%2F2a+%2B+v1%28T+-+%28v1-v0%29%2Fa%29+for+a
	class Locomotive : public Shared_Ptr<Locomotive>
	{
	public:
		struct Callbacks
		{
			using DriveCallback = std::function<const Result(const Address address, const unsigned char speed, const bool forward)>;
			DriveCallback drive = [](const Address address, const unsigned char speed, const bool forward) -> const Result {
				logger.warn("Locomotive::DriveCallback used but not implemented");
				return Result::NotImplemented;
			};

			using FunctionsCallback = std::function<const Result(const Address address, const uint32_t functions)>;
			FunctionsCallback functions = [](const Address address, const uint32_t functions) -> const Result {
				logger.warn("Locomotive::FunctionsCallback used but not implemented");
				return Result::NotImplemented;
			};

			using SignalPassedCallback = std::function<const Result(const Locomotive::Const loco, const Track::Const track, const Track::Connection connection, const Signal::Pass pass)>;
			SignalPassedCallback signalPassed = [](const Locomotive::Const loco, const Track::Const track, const Track::Connection connection, const Signal::Pass pass) -> const Result {
				logger.warn("Locomotive::SignalPassedCallback used but not implemented");
				return Result::NotImplemented;
			};

			using LocoStopped = std::function<void(const Locomotive::Const)>;
			LocoStopped stopped = [](const Locomotive::Const) {
				logger.warn("Locomotive::LocoStopped used but not implemented");
				return Result::NotImplemented;
				};
		};

		enum class Type : unsigned char
		{
			Single = 0b1,
			Passenger = 0b10,
			Goods = 0b100,
			Shunting = 0b1000
		};

		using Types = unsigned char;

		struct Function
		{
			unsigned int id;
			std::string name;
		};
		using Functions = std::vector<Function>;
		
		Locomotive(const Callbacks callbacks, const Address address, const Functions functions, const Position start, const ThrottleSpeedMap speedMap, const std::string name, const Length length, const Types types);
		inline void light(bool on);
		const bool light();
		inline void function(const unsigned int id, const bool value);
		const bool function(const unsigned int id);
		const Functions& functions();
		const bool forward();
		void reverse();
		const Speed speed();
		const Throttle throttle();
		template<bool _force>
		void drive(const bool forward, const Throttle throttle)
		{
			if (forward != this->details.forward)
			{
				this->reverse();
			}
			this->details.forward = forward;
			this->details.modelThrottle = throttle;
			if (_force)
				this->details.throttle = throttle;
		}
		void speedTrap(const Distance distance = 0);
		using EachSpeedMapCallback = std::function<void(const Throttle, const Speed)>;
		void eachSpeedMap(EachSpeedMapCallback callback) const;
		void setSpeedMap(ThrottleSpeedMap throttleSpeedMap);
		//void position(const Position p);
		const Position& position() const;
		void stop();
		const Result update(Position::Transit& transit);
		const Result update();
		const Address& address() const;
		const std::string& name() const;

		const bool isRailed() const;
		void railOnto(const Position p, const TimePoint when = hal::now());
		void railOff();

		void entered(winston::Segment::Shared segment, const TimePoint when);
		void left(winston::Segment::Shared segment, const TimePoint when);

		const bool isType(const Type type) const;
		const Types types() const;

		static const ThrottleSpeedMap defaultThrottleSpeedMap;
		void update(const bool busy, const bool forward, const Throttle throttle, const uint32_t functions);

		const NextSignal::Const nextSignal(const Signal::Pass pass, const bool forward) const;
		const bool isNextSignal(const Signal::Const signal) const;
		void updateNextSignals();

		void autodrive(const bool halt, const bool drive, const bool updateSpeedMap, const bool disappearTimeout);
	private:

		const Position& moved(Duration& timeOnTour, Position::Transit& transit);
		static const float acceleration(const Throttle throttle);

		void updateSpeed(const Throttle throttle, Position::Transit& transit);

		//void updateExpected(const bool fullUpdate = true);
#ifdef WINSTON_TEST
	public:
#endif
		class SpeedMap
		{
		public:
			SpeedMap();
			SpeedMap(ThrottleSpeedMap map);
			~SpeedMap() = default;
			const Speed speed(const Throttle throttle) const;
			const Throttle throttle(const Speed speed) const;
			void learn(const Throttle throttle, const Speed speed);
			void each(EachSpeedMapCallback callback) const;
		private:
			// unsigned char throttle <-> float speed mm/s
			ThrottleSpeedMap map;
		};

		const Callbacks callbacks;
		struct Details
		{
			Address address{ 0 };
			const Functions functionTable;
			Position position;
			TimePoint lastPositionUpdate, lastSpeedUpdate, lastEnteredTime;
			std::string name{ "" };
			const Length length{ 0 };
			bool busy{ false };
			bool forward{ true };
			bool railed{ false };
			bool trackable{ false };
			Throttle throttle{ 0 };
			Throttle modelThrottle{ 0 };
			uint32_t functions{ 0 };
			Types types{ (unsigned char)Type::Single };
			NextSignals nextSignals;
			struct
			{
				bool halt;
				bool drive;
				bool updateSpeedMap;
				bool disappearTimeout;
			} autodrive;
			bool speedTrapping{ false };
			Distance distanceSinceSpeedTrapped{ 0 };
			Segment::Shared lastEnteredSegment{ nullptr };
			bool positionUpdateRequired{ false };
		} details;

		struct Expected
		{
			Position position;
			TimePoint when;
			bool precise;
		} expected;

		SpeedMap speedMap;
		TimePoint speedTrapStart;
	};

	class LocomotiveShed
	{
	public:
		using TrackFromIndexCallback = std::function<Track::Const(const unsigned int trackIndex)>;

		LocomotiveShed();
		~LocomotiveShed() = default;

		const Result init(hal::StorageInterface::Shared storage);
		const Result add(Locomotive::Shared loco);
		Locomotive::Shared get(const Address dccAddress) const;
		const Result store(const Locomotive::Const loco);
		const Result load(Locomotive::Shared loco, TrackFromIndexCallback trackFromIndex) const;

		const std::vector<Locomotive::Shared>& shed() const;

#ifndef WINSTON_TEST
	private:
#endif
		const Result format();
		const size_t offset(const size_t count) const;
		const Result checkHeader(uint8_t& locoCount) const;
		const Result getLocoMemoryAddress(const winston::Locomotive::Const loco, size_t& address) const;

		std::vector<Locomotive::Shared> _shed;
		hal::StorageInterface::Shared storage;
	};

	class RailCar : public Shared_Ptr<RailCar>
	{
	public:
		class Groups
		{
		public:
			using Group = unsigned int;
			static const Group create()
			{
				return 1 << ++RailCar::Groups::groupCounter;
			}

			static constexpr Group CannotBeSingle = 1 << 1;
			static constexpr Group Person = 1 << 2;
			static constexpr Group Goods = 1 << 3;
			static constexpr Group ConstructionTrain = 1 << 4;
			static constexpr Group Heavy = 1 << 5;
			static constexpr Group _NextGroupCounterValue = 1 << 6;
		private:
			static unsigned int groupCounter;
		};

		RailCar(const std::string name, const Groups::Group groups, const Length length);
		~RailCar() = default;

		const bool is(const Groups::Group group) const;

		using Shared_Ptr<RailCar>::Shared;
		using Shared_Ptr<RailCar>::make;
		using Shared_Ptr<RailCar>::enable_shared_from_this_virtual::shared_from_this;

		const std::string name;
		const Groups::Group groups;
		const Length length;
	};
	using RailCarShed = std::vector<RailCar::Shared>;
}

