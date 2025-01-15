
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"
#include "SignalTower.h"

namespace winston
{
	const ThrottleSpeedMap Locomotive::defaultThrottleSpeedMap = { {0, 0.f},{255, 50.f} };
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Functions functionTable, const Position start, const ThrottleSpeedMap throttleSpeedMap, const std::string name, const Length length, const Types types) 
		: callbacks(callbacks)
		, details{ address, functionTable, start, hal::now(), hal::now(), name, length, false, true, false, 0, 0, 0, types, {}, { false, false, false }, false, 0, nullptr, false }
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

	const Speed Locomotive::speed()
	{
		return this->speedMap.speed(this->details.throttle);
	}

	const Throttle Locomotive::throttle()
	{
		return this->details.throttle;
	}

	void Locomotive::speedTrap(const Distance distance)
	{
		if (distance == 0)
		{
			this->speedTrapStart = hal::now();
			this->details.speedTrapping = true;
			this->details.distanceSinceSpeedTrapped = 0;
		}
		else if(this->details.autodrive.updateSpeedMap)
		{
			auto time = inMilliseconds(hal::now() - this->speedTrapStart);
			// x mm in y us = x/y mm/us <=> 1000x/y mm/s
			this->speedMap.learn(this->throttle(), (const Speed)((1000.f * std::abs(distance)) / time));
		}
	}

	void Locomotive::eachSpeedMap(EachSpeedMapCallback callback) const
	{
		this->speedMap.each(callback);
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
		SignalTower::setSignalsForLoco(this->const_from_this());
	}

	void Locomotive::railOff()
	{
		this->details.railed = false;
	}

	const NextSignal::Const Locomotive::nextSignal(const Signal::Pass pass, const bool forward) const
	{
		return this->details.nextSignals.get(forward, pass);
	}

	const bool Locomotive::isNextSignal(const Signal::Const signal) const
	{
		return this->details.nextSignals.contains(signal);
	}

	void Locomotive::entered(Segment::Shared segment, const TimePoint when)
	{
		if (this->position().valid())
		{
			auto currentTrack = this->position().track();
			if (segment == this->details.lastEnteredSegment)
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

						// distance we drove since the last check (mm/s * us / 1000)
						auto distanceTraveled = (speed * inMilliseconds(driveDuration)) / 1000.f;

						// minus the distance on the old track
						auto remainingDistanceOnNewTrack = distanceTraveled;
						
						// our position is already on the new track
						if (this->details.position.track() == expectedTrack)
							remainingDistanceOnNewTrack = this->position().distance();
						else // if not, substract the remaining of the old track
							remainingDistanceOnNewTrack -= (this->details.position.track()->length() - this->details.position.distance());
						// but never negative distance
						if (remainingDistanceOnNewTrack < 0)
							remainingDistanceOnNewTrack = 0;

						// so the new position is on the expected track/connection on the remainingDistance
						auto newPos = Position(expectedTrack, this->expected.position.connection(), remainingDistanceOnNewTrack);

						auto leavingConnection = currentTrack->otherConnection(this->position().connection());
						if (auto signal = currentTrack->signalGuarding(leavingConnection))
						{
							if (this->details.position.track()->length() - this->details.position.distance() < signal->distance())
							{
								// we passed signal, it was facing us and we entered its protectorate
								this->callbacks.signalPassed(currentTrack, leavingConnection, Signal::Pass::Facing);
							}
						}

						if (auto signal = expectedTrack->signalGuarding(this->expected.position.connection()))
						{
							if (signal->distance() < remainingDistanceOnNewTrack)
							{
								// we passed signal, we saw the back side and we left its protectorate
								this->callbacks.signalPassed(expectedTrack, this->expected.position.connection(), Signal::Pass::Backside);
							}
						}

						this->details.position = newPos;
						this->details.lastPositionUpdate = when;

						this->updateExpected();

						// initialize speed trap
						this->speedTrap();
					}
					else
					{
						; // we are not expected here!
					}
				}
				else // second step after initial appearance
				{
					this->updateExpected();

					// initialize speed trap
					this->speedTrap();
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
		this->details.lastEnteredSegment = segment;
	}

	void Locomotive::left(Segment::Shared segment, const TimePoint when)
	{
		// ok, cu soon
		if (this->details.speedTrapping)
		{
			this->speedTrap(this->details.distanceSinceSpeedTrapped);
		}
	}

	void Locomotive::updateExpected(const bool fullUpdate)
	{

		auto track = this->position().track();
		if (fullUpdate)
		{
			Track::Const onto;
			Track::Connection ontoConnection;
			if (track->traverseToNextSegment(this->position().connection(), false, onto, ontoConnection))
			{
				this->expected.position = Position(onto, ontoConnection, 0);
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

	void Locomotive::updateSpeed(const Throttle throttle)
	{
		Duration timeOnTour;
		Position::Transit transit;
		this->details.positionUpdateRequired = true;
		this->moved(timeOnTour, transit);
		this->details.throttle = this->details.modelThrottle = throttle;
		this->callbacks.drive(this->address(), (unsigned char)this->details.modelThrottle, this->details.forward);
		this->details.lastSpeedUpdate = hal::now();
	}

	void Locomotive::updateNextSignals()
	{
		if(this->position().track())
		{
			//second railOnto overwrites with wrong nextSignals?
			// forwards
			this->details.nextSignals.put(SignalTower::nextSignal(this->position(), Signal::Pass::Facing), true, Signal::Pass::Facing);
			this->details.nextSignals.put(SignalTower::nextSignal(this->position(), Signal::Pass::Backside), true, Signal::Pass::Backside);

			// backwards
			auto pos = this->position();
			pos.useOtherRef();
			this->details.nextSignals.put(SignalTower::nextSignal(pos, Signal::Pass::Facing), false, Signal::Pass::Facing);
			this->details.nextSignals.put(SignalTower::nextSignal(pos, Signal::Pass::Backside), false, Signal::Pass::Backside);
		}
	}

	void Locomotive::update(const bool busy, const bool forward, const Throttle throttle, const uint32_t functions)
	{
		this->details.busy = busy;
		this->details.forward = forward;
		this->details.throttle = throttle;
		this->details.modelThrottle = throttle;
		this->details.functions = functions;
	}

	const Position& Locomotive::moved(Duration& timeOnTour, Position::Transit &transit)
	{
		auto now = hal::now();
		timeOnTour = now - this->details.lastPositionUpdate;
		transit = Position::Transit::Stay;
		if (inMilliseconds(timeOnTour) > WINSTON_LOCO_POSITION_TRACK_RATE || this->details.positionUpdateRequired)
		{
			if (this->details.throttle != 0)
			{
				const auto distance = (Distance)((this->details.forward ? 1 : -1) * this->speedMap.speed(this->details.throttle) * inMilliseconds(timeOnTour)) / 1000;
				this->details.distanceSinceSpeedTrapped += distance;
				transit = this->details.position.drive(distance, this->callbacks.signalPassed);
			}
			this->details.lastPositionUpdate = now;
			this->details.positionUpdateRequired = false;
		}
		return this->position();
	}

	void Locomotive::autodrive(const bool halt, const bool drive, const bool updateSpeedMap)
	{
		this->details.autodrive.halt = halt;
		this->details.autodrive.drive = drive;
		this->details.autodrive.updateSpeedMap = updateSpeedMap;
	}

	const Result Locomotive::update()
	{
		Position::Transit transit;
		return this->update(transit);
	}

	const Result Locomotive::update(Position::Transit& transit)
	{
		if (!this->isRailed())
			return Result::NotFound;

		auto now = hal::now();
		Duration speedUpdateRate = now - this->details.lastSpeedUpdate;
		const auto timeDiff = inMilliseconds(speedUpdateRate);
		if (timeDiff > WINSTON_LOCO_SPEED_TRACK_RATE)
		{
			auto diff = (float)this->details.throttle - this->details.modelThrottle;

			if (diff != 0)
			{
				this->details.speedTrapping = false;
				// 0..255 in 4s <=> on linear curve
				// 255..0 in 2s <=> on linear curve

				if (diff > 0)
					this->details.modelThrottle += (diff / 255.f) * timeDiff / 4000.f;
				else
					this->details.modelThrottle += (diff / 255.f) * timeDiff / 2000.f;
				this->details.modelThrottle = clamp<float>(0.f, 255.f, this->details.modelThrottle);
				this->callbacks.drive(this->address(), (unsigned char)this->details.modelThrottle, this->details.forward);
			}
			this->details.lastSpeedUpdate = now;
		}
		// don't care about the updated duration
		this->moved(speedUpdateRate, transit);
		this->updateNextSignals();

		if (this->details.autodrive.halt)
		{
			// distance to next signal, forward, facing us
			if (auto nextSignal = this->details.nextSignals.get(true, Signal::Pass::Facing))
			{
				if (auto signal = nextSignal->signal)
					if (signal->shows(Signal::Aspect::Halt))
					{
						const auto speed = this->speed();
						const auto minBreaking = 0.5f;
						const auto maxBreakingDeceleration = 17.24f; // realworld: 1.5m/s <-> 17mm/s in 1:87
						const auto breakingDistance = speed * speed / (2.f * (0.9f * maxBreakingDeceleration));
						const auto decelerationToNextSignal = (float)(speed * speed) / (2.f * nextSignal->distance);
						/*
						if (nextSignal->distance < 10)
						{
							// fullstop
							this->updateSpeed(0);
						}
						else*/ if (decelerationToNextSignal > minBreaking)
						{
							const auto targetSpeedToHalt = this->speed() - (decelerationToNextSignal * timeDiff / 1000.f);
							this->updateSpeed(this->speedMap.throttle(targetSpeedToHalt));
						}
					}
			}
		}

		return Result::OK;
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

	// mm/s
	const Speed Locomotive::SpeedMap::speed(const Throttle throttle) const
	{
		auto lookup = this->map.find(throttle);
		if (lookup == this->map.end())
		{
			auto lower = this->map.lower_bound(throttle);
			auto upper = this->map.upper_bound(throttle);

			lower--;

			// throttle == lower => 0 ==> lower
			// throttle == upper => 1 ==> upper
			float frac = (float)(throttle - lower->first) / (float)(upper->first - lower->first);

			// linear
			return lerp<Speed>(lower->second, upper->second, frac);
		}
		else
			return lookup->second;
	}

	const Throttle Locomotive::SpeedMap::throttle(const Speed speed) const
	{
		for (auto it = this->map.begin(); it != this->map.end();)
		{
			const auto current = *it;
			if (++it == this->map.end())
				return current.first;

			const auto next = *it;

			if (current.second <= speed && speed <= next.second)
			{
				float frac = (float)(speed - current.second) / (float)(next.second - current.second);
				return lerp<Throttle>(current.first, next.first, frac);
			}
		}

		return 0;
	}

	void Locomotive::SpeedMap::each(EachSpeedMapCallback callback) const
	{
		for (auto const& [throttle, speed] : this->map) {
			callback(throttle, speed);
		}
	}

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