
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
	const Locomotive::ThrottleSpeedMap Locomotive::defaultThrottleSpeedMap = { {0, 0},{255, 50} };
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Functions functionTable, const Position start, const ThrottleSpeedMap throttleSpeedMap, const std::string name, const Types types) :
		callbacks(callbacks), details{ address, functionTable, start, hal::now(), hal::now(), name, false, true, 0, 0.f, 0, types }, speedMap(throttleSpeedMap), speedTrapStart(hal::now())
	{
	}

	void Locomotive::light(bool on)
	{
		this->function(0, on);
	}

	const bool Locomotive::light()
	{
		return this->function(0);
	}

	void Locomotive::function(const unsigned int id, const bool value)
	{
#define SET_nth_BIT(data, bit, value) (data & ~(1UL << bit)) | ((value) << bit)
		this->details.functions = SET_nth_BIT(this->details.functions, id, value ? 1 : 0);
		this->callbacks.functions(this->address(), this->details.functions);
#undef SET_nth_BIT
	}

	const bool Locomotive::function(const unsigned int id)
	{
		return this->details.functions & (1 << id);
	}

	const Locomotive::Functions& Locomotive::functions()
	{
		return this->details.functionTable;
	}

	const bool Locomotive::forward()
	{
		return this->details.forward;
	}

	const Locomotive::Speed Locomotive::speed()
	{
		return this->speedMap.speed(this->details.throttle);
	}

	const Locomotive::Throttle Locomotive::throttle()
	{
		return this->details.throttle;
	}

	void Locomotive::speedTrap(const Distance distance)
	{
		if (distance == 0)
		{
			this->speedTrapStart = hal::now();
		}
		else
		{
			auto time = inMilliseconds(hal::now() - this->speedTrapStart);
			this->speedMap.learn(this->throttle(), (const Locomotive::Speed)((1000*std::abs(distance)) / time));
		}
	}

	void Locomotive::stop()
	{
		this->details.throttle = 0;
	}

	void Locomotive::update(const bool busy, const bool forward, const Throttle throttle, const uint32_t functions)
	{
		this->details.busy = busy;
		this->details.forward = forward;
		this->details.throttle = throttle;
		this->details.modelThrottle = throttle;
		this->details.functions = functions;
	}

	const Position& Locomotive::moved(Duration& timeOnTour)
	{
		auto now = hal::now();
		timeOnTour = now - this->details.lastPositionUpdate;
		if (inMilliseconds(timeOnTour) > WINSTON_LOCO_POSITION_TRACK_RATE)
		{
			if(this->details.throttle != 0)
				this->details.position.drive((Distance)((this->details.forward ? 1 : -1) * this->speedMap.speed(this->details.throttle) * inMilliseconds(timeOnTour)) / 1000);
			this->details.lastPositionUpdate = now;
		}
		return this->position();
	}

	const Position& Locomotive::update()
	{
		auto now = hal::now();
		Duration speedUpdateRate = now - this->details.lastSpeedUpdate;
		if (inMilliseconds(speedUpdateRate) > WINSTON_LOCO_SPEED_TRACK_RATE)
		{
			auto diff = (float)this->details.throttle - this->details.modelThrottle;

			// 0..255 in 4s <=> on linear curve

			this->details.modelThrottle += (diff / 255.f) * inMilliseconds(speedUpdateRate) / 4000.f;
			this->details.modelThrottle = clamp<float>(0.f, 255.f, this->details.modelThrottle);
			this->callbacks.drive(this->address(), (unsigned char)this->details.modelThrottle, this->details.forward);
			this->details.lastSpeedUpdate = now;
		}
		// dont care about the updated duration
		return this->moved(speedUpdateRate);
	}

	void Locomotive::position(const Position p)
	{
		this->details.lastPositionUpdate = hal::now();
		this->details.position = p;
	}

	const Position& Locomotive::position() const
	{
		return this->details.position;
	}

	const Address& Locomotive::address() const
	{
		return this->details.address;
	}

	const std::string& Locomotive::name()
	{
		return this->details.name;
	}

	const bool Locomotive::isType(const Type type) const
	{
		return (unsigned char)this->details.types & (unsigned char)type;
	}

	const Locomotive::Types Locomotive::types() const
	{
		return this->details.types;
	}

	const float Locomotive::acceleration(const Throttle throttle)
	{
		float x = throttle / 255.f;
		if (x < 0.5f)
			return 1.f;
		else if (x < 0.8f)
			return -x * 2.f + 2.f;
		else if (x < 1.f)
			return -x + 1.2f;
		else 
			return 0.f;
	}

	Locomotive::SpeedMap::SpeedMap()
		: map()
	{

	}

	Locomotive::SpeedMap::SpeedMap(ThrottleSpeedMap map)
		: map(map)
	{

	}

	void Locomotive::SpeedMap::learn(const Throttle throttle, const Speed speed)
	{
		if (this->map.find(throttle) == this->map.end())
		{
			this->map.insert(std::make_pair(throttle, speed));
		}
		else
			this->map[throttle] = speed;
	}
	const Locomotive::Speed Locomotive::SpeedMap::speed(const Throttle throttle) const
	{
		auto lookup = this->map.find(throttle);
		if (lookup == this->map.end())
		{
			auto lower = this->map.lower_bound(throttle);
			auto upper = this->map.upper_bound(throttle);

			// throttle == lower => 0 ==> lower
			// throttle == upper => 1 ==> upper
			float frac = (float)(throttle - lower->first) / (float)(upper->first - lower->first);
			
			// linear
			return lerp<Speed>(lower->second, upper->second, frac);
		}
		else
			return lookup->second;
	}
	/*
	const Locomotive::Speed Locomotive::SpeedMap::lerp(Locomotive::Speed lower, Locomotive::Speed upper, const float frac)
	{
		if (frac <= 0.0f)
			return lower;
		else if (frac >= 1.0f)
			return upper;
		return (Speed)(frac * upper + (1.0f - frac) * lower);
	}*/
}