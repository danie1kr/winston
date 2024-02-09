#pragma once

#include <map>
#include "WinstonTypes.h"
#include "Position.h"
#include "Track.h"
#include "Log.h"

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

		s = v0 * (ta-t0) + a/2 * (tb-ta)² + v1 * (t1-tb)
		s = v0 * (T0) + a/2 * (Ta)² + v1 * (T1)
		===================================================
		v1    /------
			 /
		v0 -/
		 0
			| |    |
			t0ta   t1

		known: v0, v1, s, t0, t1
		a = (v1 - v0) / (ta - t0)
		s = v0 * (ta-t0) + a/2 * (ta-t0)² + v1 * (t1-ta)
		s = v0 * (ta-t0) + 1/2 * (v1 - v0) / (ta - t0) * (ta-t0)² + v1 * (t1-ta)
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
			using DriveCallback = std::function<void(const Address address, const unsigned char speed, const bool forward)>;
			DriveCallback drive = [](const Address address, const unsigned char speed, const bool forward) {
				logger.warn("Locomotive::DriveCallback used but not implemented");
				return winston::State::Finished;
			};

			using FunctionsCallback = std::function<void(const Address address, const uint32_t functions)>;
			FunctionsCallback functions = [](const Address address, const uint32_t functions) {
				logger.warn("Locomotive::FunctionsCallback used but not implemented");
				return winston::State::Finished;
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
		using Throttle = unsigned char;
		using Speed = unsigned int;
		using ThrottleSpeedMap = std::map<Throttle, Speed>;

		struct Function
		{
			unsigned int id;
			std::string name;
		};
		using Functions = std::vector<Function>;
		
		Locomotive(const Callbacks callbacks, const Address address, const Functions functions, const Position start, const ThrottleSpeedMap speedMap, const std::string name, const unsigned int length, const Types types);
		inline void light(bool on);
		const bool light();
		inline void function(const unsigned int id, const bool value);
		const bool function(const unsigned int id);
		const Functions& functions();
		const bool forward();
		const Speed speed();
		const Throttle throttle();
		template<bool _force>
		void drive(const bool forward, const Throttle throttle)
		{
			this->details.forward = forward;
			this->details.modelThrottle = throttle;
			if (_force)
				this->details.throttle = throttle;
		}
		void speedTrap(const Distance distance = 0);
		const Position& moved(Duration& timeOnTour);
		void position(const Position p);
		const Position& position() const;
		void stop();
		const Position& update();
		const Address& address() const;
		const std::string& name();


		const bool isType(const Type type) const;
		const Types types() const;

		static const ThrottleSpeedMap defaultThrottleSpeedMap;
		void update(const bool busy, const bool forward, const Throttle throttle, const uint32_t functions);
	private:

		static const float acceleration(const Throttle throttle);

		class SpeedMap
		{
		public:
			SpeedMap();
			SpeedMap(ThrottleSpeedMap map);
			const Speed speed(const Throttle throttle) const;
			void learn(const Throttle throttle, const Speed speed);
		private:
			//static const Speed lerp(const Speed lower, const Speed upper, const float frac);
			ThrottleSpeedMap map;
		};

		const Callbacks callbacks;
		struct Details
		{
			Address address = { 0 };
			const Functions functionTable;
			Position position;
			TimePoint lastPositionUpdate, lastSpeedUpdate;
			std::string name = { "" };
			const Length length;
			bool busy = { false };
			bool forward = { true };
			Throttle throttle = { 0 };
			float modelThrottle = { 0.f };
			uint32_t functions = { 0 };
			Types types = { (unsigned char)Type::Single };
		} details;

		SpeedMap speedMap;
		TimePoint speedTrapStart;
	};
	using LocomotiveShed = std::vector<Locomotive::Shared>;

	class RailCar : public Shared_Ptr<RailCar>
	{
	public:
		class Groups
		{
		public:
			using Group = unsigned int;
			static constexpr Group create()
			{
				return 1 << ++RailCar::Groups::groupCounter;
			}

			static constexpr Group Person = 1 << 0;
			static constexpr Group Goods = 1 << 1;
			static constexpr Group ConstructionTrain = 1 << 2;
			static constexpr Group Heavy = 1 << 3;
		private:
			static unsigned int groupCounter;
		};

		RailCar(const std::string name, const Groups::Group groups, const Length length);
		~RailCar() = default;

		const bool is(Groups::Group group) const;

		using Shared_Ptr<RailCar>::Shared;
		using Shared_Ptr<RailCar>::make;
		using Shared_Ptr<RailCar>::enable_shared_from_this_virtual::shared_from_this;

		const std::string name;
		const Groups::Group groups;
		const Length length;
	};
	using RailCarShed = std::vector<RailCar::Shared>;
}

