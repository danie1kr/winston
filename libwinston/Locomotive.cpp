
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"
#include "SignalTower.h"

namespace winston
{
	const Locomotive::ThrottleSpeedMap Locomotive::defaultThrottleSpeedMap = { {0, 0},{255, 50} };
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Functions functionTable, const Position start, const ThrottleSpeedMap throttleSpeedMap, const std::string name, const unsigned int length, const Types types) 
		: callbacks(callbacks)
		, details{ address, functionTable, start, hal::now(), hal::now(), name, length, false, true, false, 0, 0.f, 0, types }
		, expected { Position::nullPosition(), hal::now(), false }
		, speedMap(throttleSpeedMap)
		, speedTrapStart(hal::now())
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

	const bool Locomotive::isRailed() const
	{
		return this->details.railed;
	}

	void Locomotive::railOnto(const Position p, const TimePoint when)
	{
		this->details.lastPositionUpdate = when;
		this->details.position = p;
		this->details.railed = true;

		this->updateNextSignals();
	}

	void Locomotive::railOff()
	{
		this->details.railed = false;
	}

	const bool Locomotive::isNextSignal(Signal::Const signal) const
	{
		return this->details.nextSignals.contains(signal);
	}

	void Locomotive::entered(Segment::Shared segment, const TimePoint when)
	{
		if (this->position().valid())
		{
			auto currentTrack = this->position().track();
			if (segment->contains(*currentTrack))
			{
				;// we appeared on our current position?
			}
			else
			{
				// known for long
				auto expectedTrack = this->expected.position.track();
				if (this->expected.precise && expectedTrack)
				{
					if (segment->contains(*expectedTrack))
					{
						auto driveDuration = when - this->details.lastPositionUpdate;
						auto speed = this->speedMap.speed(this->details.throttle);
						auto remainingDistance = speed * inMilliseconds(driveDuration);

						remainingDistance -= (this->details.position.track()->length() - this->details.position.distance());

						auto newPos = Position(expectedTrack, this->expected.position.connection(), (int)remainingDistance);

						auto leavingConnection = currentTrack->otherConnection(this->position().connection());
						if (auto signal = currentTrack->signalGuarding(leavingConnection))
						{
							if (this->details.position.track()->length() - this->details.position.distance() > signal->distance())
							{
								// we passed signal, it was facing us and we entered its protectorate
								this->callbacks.signalPassed(currentTrack, leavingConnection, Signal::Pass::Facing);
							}
						}

						if (auto signal = expectedTrack->signalGuarding(this->expected.position.connection()))
						{
							if (signal->distance() < remainingDistance)
							{
								// we passed signal, we saw the back side and we left its protectorate
								this->callbacks.signalPassed(expectedTrack, this->expected.position.connection(), Signal::Pass::Backside);
							}
						}

						this->details.position = newPos;
						this->details.lastPositionUpdate = when;

						this->updateExpected();
					}
					else
					{
						; // we are not expected here!
					}
				}
				else // second step after initial appearance
				{
					this->updateExpected();
				}
			}
		}
		else // initial appearance
		{
			// put onto any rail of the segment for now
			this->railOnto(Position(*segment->tracks().begin(), Track::Connection::A, 0), when);
			this->expected.position = Position(nullptr, Track::Connection::A, 0);
			this->expected.precise = false;
		}
	}

	void Locomotive::left(Segment::Shared segment, const TimePoint when)
	{
		// ok, cu soon
	}

	void Locomotive::updateExpected(const bool fullUpdate)
	{

		auto track = this->position().track();
		if (fullUpdate)
		{
			Track::Const onto;
			if (track->traverseToNextSegment(this->position().connection(), onto, false))
			{
				this->expected.position = Position(onto, onto->whereConnects(track), 0);
				this->expected.precise = true;
			}
			else
			{
				// seems we hit a deadend/derail situation
				this->expected.position = Position(track, Track::Connection::DeadEnd, 0);
				this->expected.precise = false;
			}
		}
		if (this->expected.precise)
		{
			auto speed = this->speedMap.speed(this->details.throttle);
			auto remainingDistance = this->details.position.track()->length() - this->details.position.distance();
			if (speed > 0)
			{
				auto travelTime = toMilliseconds(remainingDistance / speed);
				this->expected.when = hal::now() + travelTime;
			}
			else
			{
				this->expected.when = hal::now() + toSeconds(60 * 60 * 24);
			}

		}
	}

	void Locomotive::updateNextSignals()
	{/*
		{
			// forwards
			this->details.nextSignals[0] = SignalTower::nextSignals(this->position(), Signal::Pass::Facing);
		}
		{
			// backwards
			auto pos = this->position();
			pos.useOtherRef();
			this->details.nextSignals[1] = SignalTower::nextSignals(pos, Signal::Pass::Facing);
		}
		
		{
			// forwards
			auto current = this->position().track();
			auto connection = current->otherConnection(this->position().connection());

			if (auto signal = current->signalGuarding(connection))
			{
				this->details.nextSignals[0] = signal;
			}
			else
			{
				this->details.nextSignals[0] = SignalTower::nextSignal(current, true, connection, true, true);
			}
		}

		{
			//backwards
			auto current = this->position().track();
			auto connection = this->position().connection();

			if (auto signal = current->signalGuarding(connection))
			{
				this->details.nextSignals[1] = signal;
			}
			else
			{
				this->details.nextSignals[1] = SignalTower::nextSignal(current, true, connection, true, true);
			}
		}
		*/
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
				this->details.position.drive((Distance)((this->details.forward ? 1 : -1) * this->speedMap.speed(this->details.throttle) * inMilliseconds(timeOnTour)) / 1000, this->callbacks.signalPassed);
			this->details.lastPositionUpdate = now;
		}
		return this->position();
	}

	const Result Locomotive::update()
	{
		if (!this->isRailed())
			return Result::NotFound;

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
		this->moved(speedUpdateRate);

		return Result::OK;
	}
	/*
	void Locomotive::position(const Position p)
	{
		this->details.lastPositionUpdate = hal::now();
		this->details.position = p;
	}*/

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

	RailCar::Groups::Group RailCar::Groups::groupCounter = RailCar::Groups::_NextGroupCounterValue;
	/*constexpr RailCar::Groups::Group RailCar::Groups::create()
	{
		return 1 << ++RailCar::Groups::groupCounter;
	}*/

	RailCar::RailCar(const std::string name, const Groups::Group groups, const Length length)
		: Shared_Ptr<RailCar>(), name{name}, groups{groups}, length{length}
	{

	}

	const bool RailCar::is(const Groups::Group group) const
	{
		return this->groups & group;
	}
}