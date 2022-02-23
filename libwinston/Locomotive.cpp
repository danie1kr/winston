
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Position start, const std::string name, const NFCAddress nfcAddress) :
		callbacks(callbacks), details{ address, nfcAddress, start, hal::now(), name, false, true, 0, 0 }, speedMap(), speedTrapStart(hal::now())
	{
		this->speedMap.learn(0, 0);
		this->speedMap.learn(255, 50);
	}

	void Locomotive::light(bool on)
	{
		//this->details.functions = (this->details.functions & ~(1UL << 1)) | ((on ? 1 : 0) << 1);
		//this->details.functions &= (0xFFFFFFFF & 0b1 | ;
		this->callbacks.functions(this->address(), this->details.functions);
	}

	const bool Locomotive::light()
	{
		return this->details.functions & 0b1;
	}

	const bool Locomotive::forward()
	{
		return this->details.forward;
	}

	const unsigned char Locomotive::speed()
	{
		return this->details.speed;
	}

	void Locomotive::drive(const bool forward, const unsigned char speed)
	{
		this->callbacks.drive(this->address(), speed, forward);
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
			this->speedMap.learn(this->speed(), (1000*distance) / time);
		}
	}

	void Locomotive::stop()
	{
		this->details.speed = 0;
	}

	void Locomotive::update(const bool busy, const bool forward, const unsigned char speed, const uint32_t functions)
	{
		this->details.busy = busy;
		this->details.forward = forward;
		this->details.speed = speed;
		this->details.functions = functions;
	}

	const Position& Locomotive::moved(Duration& timeOnTour)
	{
		auto now = hal::now();
		timeOnTour = now - this->details.lastPositionUpdate;
		if (inMilliseconds(timeOnTour) > WINSTON_LOCO_POSITION_TRACK_RATE)
		{
			this->details.lastPositionUpdate = now;
			this->details.position.drive((Distance)(this->speedMap.speed(this->details.speed) * inMilliseconds(timeOnTour)) / 1000);
		}
		return this->position();
	}

	void Locomotive::position(const Position p)
	{
		this->details.lastPositionUpdate = hal::now();
		this->details.position = p;
	}

	const Position& Locomotive::position()
	{
		return this->details.position;
	}

	const Address& Locomotive::address() const
	{
		return this->details.address;
	}

	const NFCAddress& Locomotive::nfcAddress() const
	{
		return this->details.nfcAddress;
	}

	const std::string& Locomotive::name()
	{
		return this->details.name;
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
	const Locomotive::SpeedMap::Speed Locomotive::SpeedMap::speed(const Throttle throttle) const
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
			return this->lerp(lower->second, upper->second, frac);
		}
		else
			return lookup->second;
	}

	const Locomotive::SpeedMap::Speed Locomotive::SpeedMap::lerp(Locomotive::SpeedMap::Speed lower, Locomotive::SpeedMap::Speed upper, const float frac)
	{
		if (frac <= 0.0f)
			return lower;
		else if (frac >= 1.0f)
			return upper;
		return (Speed)(frac * upper + (1.0f - frac) * lower);
	}
}