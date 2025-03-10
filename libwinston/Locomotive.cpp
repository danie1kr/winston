
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"
#include "SignalTower.h"

/*
* BR218
Throttle	Speed
0	0.000000 mm/s
23	94.945419 mm/s
33	188.490250 mm/s
37	9.036855 mm/s
38	238.369415 mm/s
45	329.617828 mm/s
255	50.000000 mm/s
*/

namespace winston
{
	const ThrottleSpeedMap Locomotive::defaultThrottleSpeedMap = { {0, 0.f} };
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Functions functionTable, const Position start, const ThrottleSpeedMap throttleSpeedMap, const std::string name, const Length length, const Types types)
		: callbacks(callbacks)
		, details{ address, functionTable, start, hal::now(), hal::now(), hal::now(), name, length, false, true, false, false, 0, 0, 0, types, {}, { false, false, false, false }, false, 0, nullptr, nullptr, Track::Connection::DeadEnd, false }
		, expected{ Position::nullPosition(), hal::now(), false }
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

	void Locomotive::reverse()
	{
#pragma message("does this work? No test yet")
#pragma message("only accept reverse if speed = 0")
		this->details.forward = !this->details.forward;
		this->details.position.useOtherRef();
		this->details.nextSignals.reverse();
		//this->updateExpected();
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
		if (distance == 0.f)// && !this->details.speedTrapping)
		{
			this->speedTrapStart = hal::now();
			this->details.speedTrapping = true;
			this->details.distanceSinceSpeedTrapped = 0;
		}
		else if (distance != 0.f && this->details.speedTrapping && this->details.autodrive.updateSpeedMap && this->throttle() > 0)
		{
			auto time = inMilliseconds(hal::now() - this->speedTrapStart);
			// x mm in y us = x/y mm/us <=> 1000x/y mm/s
			auto speed = (const Speed)((1000.f * std::abs(distance)) / time);
			this->speedMap.learn(this->throttle(), speed);
			LOG_INFO("Loco ", this->name(), " (", this->address(), ") speedtrapped for ", distance, "mm with ", speed, "mm/s");
			this->speedTrap(0.f);
		}
	}

	void Locomotive::eachSpeedMap(EachSpeedMapCallback callback) const
	{
		this->speedMap.each(callback);
	}

	void Locomotive::setSpeedMap(ThrottleSpeedMap throttleSpeedMap)
	{
		this->speedMap = throttleSpeedMap;
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
		this->details.position = Position(nullptr, Track::Connection::A, 0.f);
		this->invalidateSpeedTrap();
		this->details.trackable = false;
		this->details.throttle = this->details.modelThrottle = 0;
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
		/*
			1. initial appearance
				railOnto
					isTrackable false
			2. loco was on a segment before
				this->details.lastEnteredSegment != null
				if(isTrackable)
					if(this->details.lastEnteredSegment.isFixedLength)
						speedTrap(this->details.lastEnteredSegment.length)

					// we think the loco is still on the old segment, so catch up signals if any
					if(this->position().track == this->details.lastEnteredSegment)
						if(signal = track.signalFacing(position.reference)
							...

					// restart speedtrap
					speedTrap();
					isTrackable = true;

			this->details.lastEnteredSegment


		*/
		const auto lastSegment = this->details.lastEnteredSegment;
		if (segment == lastSegment)
		{
			// we appeared where we are already... so?!
			int a = 0;
		}
		else if (lastSegment != nullptr)
		{
			if (this->details.trackable)
			{
				this->speedTrap(lastSegment->length(this->details.lastEnteredTrack, this->details.lastEnteredConnection));
//				if (lastSegment->fixedLength())
	//				this->speedTrap(lastSegment->length());
		//		else // calculate length of lastSegment
			//		this->speedTrap(this->details.distanceSinceSpeedTrapped);
			}

			bool foundNextSectionEntry = false;
			Track::Const entryTrack;
			Track::Connection entryConnection;
			if (this->details.trackable)
			{
				auto current = this->position().track();
				auto connection = this->position().connection();
				auto distance = this->position().distance();
				auto onto = current;

				// the signal guarding our reference connection
				if (auto signal = current->signalGuarding(connection))
				{
					if (signal->distance() >= this->details.position.distance())
					{
						// we passed signal, it was facing us and we entered its protectorate
						this->callbacks.signalPassed(this->const_from_this(), current, connection, Signal::Pass::Backside);
					}
				}

				// the signal in front of us
				const auto leavingConnection = current->otherConnection(connection);
				if (auto signal = current->signalGuarding(leavingConnection))
				{
					if (current->length() - this->details.position.distance() >= signal->distance())
					{
						// we passed signal, it was facing us and we entered its protectorate
						this->callbacks.signalPassed(this->const_from_this(), current, leavingConnection, Signal::Pass::Facing);
					}
				}

				// should not take long to go over all remaining tracks in the segment
				const size_t steps = 8;
				for (size_t i = 0; i < steps; ++i)
				{
					if (current->traverse(connection, onto, false))
					{
						connection = onto->whereConnects(current);
						if (current->segment() != onto->segment())
						{
							entryTrack = onto;
							entryConnection = connection;
							foundNextSectionEntry = true;
							break;
						}
						else
						{
							current = onto;
							// pass all signals on that track
							// the signal guarding our reference connection
							if (auto signal = current->signalGuarding(connection))
							{
								// we passed signal, it was facing us and we entered its protectorate
								this->callbacks.signalPassed(this->const_from_this(), current, connection, Signal::Pass::Backside);
							}

							// the signal in front of us
							const auto leavingConnection = current->otherConnection(connection);
							if (auto signal = current->signalGuarding(leavingConnection))
							{
								// we passed signal, it was facing us and we entered its protectorate
								this->callbacks.signalPassed(this->const_from_this(), current, leavingConnection, Signal::Pass::Facing);
							}
						}
					}
					else
						// we are lost
						break;
				}
			}

			SegmentEntry entry;
			if (!foundNextSectionEntry && segment->from(*lastSegment, entry))
			{
				entryTrack = entry.first;
				entryConnection = entry.second;
				foundNextSectionEntry = true;
			}

			if(foundNextSectionEntry)
			{
				Position entryPos(entryTrack, entryConnection, 0.f);
				/*
				if (this->details.trackable)
				{
					const auto leavingTrack = entryTrack->on(entryConnection);
					const auto leavingConnection = leavingTrack->whereConnects(entryTrack);

					if (auto signal = leavingTrack->signalGuarding(leavingConnection))
					{
						if (this->details.position.track()->length() - this->details.position.distance() <= signal->distance())
						{
							// we passed signal, it was facing us and we entered its protectorate
							this->callbacks.signalPassed(this->const_from_this(), leavingTrack, leavingConnection, Signal::Pass::Facing);
						}
					}
				}*/
				this->details.position = entryPos;
				this->details.lastPositionUpdate = hal::now();
				
				// restart speedtrap
				speedTrap();
				//this->updateExpected();
				this->details.trackable = true;
			}
			else
			{
				// we are lost, so restart
				LOG_ERROR("Loco", this->name(), " is lost on segment", segment->id, " current position: ", this->details.position.trackName(), "-", this->details.position.connection(), "+", this->details.position.distance());
				this->railOnto(Position(*segment->tracks().begin(), Track::Connection::A, 0), when);
				this->details.trackable = false;
			}
		}
		else // initial appearance
		{
			// put onto any rail of the segment for now
			this->railOnto(Position(*segment->tracks().begin(), Track::Connection::A, 0), when);
			this->details.trackable = false;
		}
		this->details.lastEnteredTime = hal::now();
		if (this->details.lastEnteredSegment)
		{
			this->details.lastEnteredTrack = this->details.position.track();
			this->details.lastEnteredConnection = this->details.position.connection();
		}
		this->details.lastEnteredSegment = segment;


		/*
		if (auto ls = this->details.lastEnteredSegment; ls && ls->fixedLength())
		{
			SegmentEntry entry;
			if (segment->from(*ls, entry))
			{
				Position entryPos(entry.first, entry.second, 0.f);
				this->details.position = entryPos;
				this->details.lastPositionUpdate = hal::now();

				we might have passed some signals!
			}
		}
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
				if (this->details.lastEnteredSegment && this->details.lastEnteredSegment->fixedLength())
					this->speedTrap(this->details.lastEnteredSegment->length);
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
								this->callbacks.signalPassed(this->const_from_this(), currentTrack, leavingConnection, Signal::Pass::Facing);
							}
						}

						if (auto signal = expectedTrack->signalGuarding(this->expected.position.connection()))
						{
							if (signal->distance() < remainingDistanceOnNewTrack)
							{
								// we passed signal, we saw the back side and we left its protectorate
								this->callbacks.signalPassed(this->const_from_this(), expectedTrack, this->expected.position.connection(), Signal::Pass::Backside);
							}
						}

						this->details.position = newPos;
						this->details.lastPositionUpdate = when;

						this->updateExpected();

						// initialize speed trap
						//this->speedTrap();
					}
					else
					{
						; // we are not expected here!
						/*
						auto newPos = Position(expectedTrack, this->expected.position.connection(), remainingDistanceOnNewTrack);

						this->details.position = newPos;
						this->details.lastPositionUpdate = when;
						this->updateExpected();
						// initialize speed trap
						this->speedTrap();
						*
					}
				}
				else // second step after initial appearance
				{
					this->updateExpected();

					// initialize speed trap
					//this->speedTrap();
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
		this->speedTrap(0.f);
		*/
	}

	void Locomotive::left(Segment::Shared segment, const TimePoint when)
	{
		// ok, cu soon
		/*if (this->details.speedTrapping)
		{
			this->speedTrap(this->details.distanceSinceSpeedTrapped);
		}*/
	}
	/*
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
	*/
	void Locomotive::updateSpeed(const Throttle throttle, Position::Transit &transit)
	{
		Duration timeOnTour;
		this->details.positionUpdateRequired = true;
		this->moved(timeOnTour, transit);

		if (throttle != this->details.throttle && throttle == 0)
			this->callbacks.stopped(this->const_from_this());

		this->details.throttle = this->details.modelThrottle = throttle;
		this->callbacks.drive(this->address(), (unsigned char)this->details.modelThrottle, this->details.forward);
		this->details.lastSpeedUpdate = hal::now();
	}

	void Locomotive::invalidateSpeedTrap()
	{
		this->details.speedTrapping = false;
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
		if(throttle != this->details.throttle && throttle == 0)
			this->callbacks.stopped(this->const_from_this());

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
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x2040);
		if (inMilliseconds(timeOnTour) > WINSTON_LOCO_POSITION_TRACK_RATE || this->details.positionUpdateRequired)
		{
			if (this->details.throttle != 0)
			{
				const auto distance = (Distance)((this->details.forward ? 1 : -1) * this->speedMap.speed(this->details.throttle) * inMilliseconds(timeOnTour)) / 1000;
			//	this->details.distanceSinceSpeedTrapped += distance;

				using namespace std::placeholders;
			//	auto currentTrack = this->position().track();
				TEENSY_CRASHLOG_BREADCRUMB(6, 0x2041);
				transit = this->details.position.drive(distance, false, std::bind(this->callbacks.signalPassed, this->const_from_this(), _1, _2, _3));
			//	if(transit == Position::Transit::CrossSection || transit == Position::Transit::CrossTrack || transit == Position::Transit::SegmentBorder)
			//		this->details.distanceSinceSpeedTrapped += currentTrack->length();
			}
			this->details.lastPositionUpdate = now;
			this->details.positionUpdateRequired = false;
		}
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x2042);
		return this->position();
	}

	void Locomotive::autodrive(const bool halt, const bool drive, const bool updateSpeedMap, const bool disappearTimeout)
	{
		this->details.autodrive.halt = halt;
		this->details.autodrive.drive = drive;
		this->details.autodrive.updateSpeedMap = updateSpeedMap;
		this->details.autodrive.disappearTimeout = disappearTimeout;
	}

	const Result Locomotive::update()
	{
		Position::Transit transit;
		return this->update(transit);
	}

	const Result Locomotive::update(Position::Transit& transit)
	{
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x200);
#ifdef WINSTON_DETECTOR_ADDRESS
		if (this->address() != WINSTON_DETECTOR_ADDRESS)
			return Result::OK;
#endif
		if (!this->isRailed())
			return Result::NotFound;

		TEENSY_CRASHLOG_BREADCRUMB(6, 0x201);
		auto now = hal::now();
		/*
		if (this->details.autodrive.disappearTimeout && now > this->details.lastEnteredTime + toMilliseconds(12000))
		{
			this->railOff();
			return Result::NotFound;
		}*/

		TEENSY_CRASHLOG_BREADCRUMB(6, 0x202);
		Duration speedUpdateRate = now - this->details.lastSpeedUpdate;
		const auto timeDiff = inMilliseconds(speedUpdateRate);
		if (timeDiff > WINSTON_LOCO_SPEED_TRACK_RATE)
		{
			auto diff = (float)this->details.throttle - this->details.modelThrottle;

			if (diff != 0)
			{
				TEENSY_CRASHLOG_BREADCRUMB(6, 0x203);
				this->invalidateSpeedTrap();
				// 0..255 in 4s <=> on linear curve
				// 255..0 in 2s <=> on linear curve

				if (diff > 0)
					this->details.modelThrottle += (Throttle)((diff / 255.f) * timeDiff / 4000.f);
				else
					this->details.modelThrottle += (Throttle)((diff / 255.f) * timeDiff / 2000.f);
				this->details.modelThrottle = (Throttle)clamp<float>(0.f, 255.f, this->details.modelThrottle);
				TEENSY_CRASHLOG_BREADCRUMB(6, 0x204);
				//this->callbacks.drive(this->address(), (unsigned char)this->details.modelThrottle, this->details.forward);
			}
			this->details.lastSpeedUpdate = now;
		}
		// don't care about the updated duration
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x204);
		this->moved(speedUpdateRate, transit);
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x205);
		this->updateNextSignals();

		TEENSY_CRASHLOG_BREADCRUMB(6, 0x207);
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
							Position::Transit updateSpeedTransit;
							this->updateSpeed(this->speedMap.throttle(targetSpeedToHalt), updateSpeedTransit);
							if (updateSpeedTransit != Position::Transit::Stay)
								transit = updateSpeedTransit;
						}
					}
			}
		}
		TEENSY_CRASHLOG_BREADCRUMB(6, 0x299);

		return Result::OK;
	}

	const Position& Locomotive::position() const
	{
		return this->details.position;
	}

	const Segment::IdentifyerType Locomotive::segment() const
	{
		if (this->position().valid())
			return this->position().track()->segment();
		else
			return 0;
	}

	const Address& Locomotive::address() const
	{
		return this->details.address;
	}

	const std::string& Locomotive::name() const
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
		if (lookup == this->map.end() && this->map.size() > 1)
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
		else if(this->map.size() == 1)
			return this->map.begin()->second;
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

	LocomotiveShed::LocomotiveShed()
	{

	}

	const Result LocomotiveShed::init(hal::StorageInterface::Shared storage)
	{
		this->storage = storage;

		uint8_t locoCount;
		auto result = this->checkHeader(locoCount);
		if (result != winston::Result::OK)
			this->format();

		return Result::OK;
	}

	const Result LocomotiveShed::format()
	{
		LOG_WARN("Formatting LocomotiveShed storage");
		size_t address = 0;
		this->storage->write(address++, (uint8_t)WINSTON_STORAGE_LOCOSHED_VERSION);
		this->storage->write(address++, (uint8_t)0);
		return Result::OK;
	}

	const Result LocomotiveShed::add(Locomotive::Shared loco)
	{
		this->_shed.push_back(loco);
		return Result::OK;
	}

	Locomotive::Shared LocomotiveShed::get(const Address dccAddress) const
	{
		if (auto it = std::find_if(this->_shed.begin(), this->_shed.end(), [&dccAddress](auto& loco)
			{
				return loco->address() == dccAddress;
			}); it != this->_shed.end())
			return *it;

		return nullptr;
	}

	const std::vector<Locomotive::Shared>& LocomotiveShed::shed() const
	{
		return this->_shed;
	}

	const Result LocomotiveShed::store(const Locomotive::Const loco)
	{
		/*
	2 address
	last position
		4 track
		1 reference connection
		4 distance
	1 speed map count
	speed map
		1 throttle
		4 speed
		*/
		if (!this->storage)
			return Result::NotInitialized;

		size_t address = 0;
		if (auto result = this->getLocoMemoryAddress(loco, address); result != winston::Result::OK)
			return result;

		// update locoCount if we are a new loco entry
		{
			uint8_t locoCount = 0;
			this->storage->read(1, locoCount);

			if (address == this->offset(locoCount))
			{
				this->storage->write(1, ++locoCount);
			}
		}

		this->storage->writeUint16(address, loco->address()); address += sizeof(decltype(loco->address()));
		const auto pos = loco->position();
		uint32_t trackIndex = pos.trackIndex();
		uint8_t connection = (uint8_t)pos.connection();
		float distance = pos.distance();
		this->storage->writeUint32(address, trackIndex); address += sizeof(decltype(trackIndex));
		this->storage->write(address, connection); address += sizeof(decltype(connection));
		this->storage->writeFloat(address, distance); address += sizeof(decltype(distance));

		uint8_t speedMapCount = 0;
		const uint32_t speedMapCountAddress = (uint32_t)address;
		address += sizeof(decltype(speedMapCount));
		loco->eachSpeedMap([&](const winston::Throttle throttle, const winston::Speed speed) {
			this->storage->write(address, throttle); address += sizeof(decltype(throttle));
			this->storage->writeFloat(address, speed); address += sizeof(decltype(speed));
			++speedMapCount;
			});
		this->storage->write(speedMapCountAddress, speedMapCount);
		this->storage->sync();

		return winston::Result::OK;
	}

	const Result LocomotiveShed::load(Locomotive::Shared loco, TrackFromIndexCallback trackFromIndex) const
	{
		if (!this->storage)
			return Result::NotInitialized;

		size_t address = 0;
		if (auto result = this->getLocoMemoryAddress(loco, address); result != winston::Result::OK)
			return result;

		uint16_t locoAddress = 0;
		auto result = this->storage->readUint16(address, locoAddress); address += sizeof(decltype(locoAddress));

		uint32_t trackIndex;
		uint8_t connection;
		float distance;
		this->storage->readUint32(address, trackIndex); address += sizeof(decltype(trackIndex));
		this->storage->read(address, connection); address += sizeof(decltype(connection));
		this->storage->readFloat(address, distance); address += sizeof(decltype(distance));

		auto track = trackFromIndex(trackIndex);
		winston::Position pos(track, (Track::Connection)connection, distance);


#ifdef WINSTON_DETECTOR_ADDRESS
		if (locoAddress == WINSTON_DETECTOR_ADDRESS)
		{
#endif
			if (pos.track())
				loco->railOnto(pos);
#ifdef WINSTON_DETECTOR_ADDRESS
		}
#endif

		uint8_t speedMapCount = 0;
		this->storage->read(address, speedMapCount); address += sizeof(decltype(speedMapCount));

		winston::ThrottleSpeedMap throttleSpeedMap;
		for (size_t i = 0; i < speedMapCount; ++i)
		{
			winston::Throttle throttle;
			winston::Speed speed;
			this->storage->read(address, throttle); address += sizeof(decltype(throttle));
			this->storage->readFloat(address, speed); address += sizeof(decltype(speed));

			throttleSpeedMap.emplace(throttle, speed);
		}
		if(speedMapCount != 0)
			loco->setSpeedMap(throttleSpeedMap);

		return winston::Result::OK;
	}

	const Result LocomotiveShed::checkHeader(uint8_t& locoCount) const
	{
		locoCount = 0;
		if (!this->storage)
			return Result::NotInitialized;

		uint8_t version = WINSTON_STORAGE_LOCOSHED_VERSION;
		size_t address = 0;
		auto result = this->storage->read(address++, version);
		if (result != winston::Result::OK || version != WINSTON_STORAGE_LOCOSHED_VERSION)
			return winston::Result::ValidationFailed;

		locoCount = 0;
		result = this->storage->read(address++, locoCount);
		if (result != winston::Result::OK)
			return winston::Result::ValidationFailed;

		return winston::Result::OK;
	}

	const Result LocomotiveShed::getLocoMemoryAddress(const winston::Locomotive::Const loco, size_t& address) const
	{
		if (!this->storage)
			return Result::NotInitialized;

		uint8_t locoCount;
		auto result = this->checkHeader(locoCount);
		if (result != winston::Result::OK)
			return result;

		for (size_t i = 0; i < locoCount; ++i)
		{
			address = this->offset(i);
			uint16_t locoAddress = 0;
			result = this->storage->readUint16(address, locoAddress);

			if (locoAddress == loco->address())
				return winston::Result::OK;
		}

		// address of new loco
		address = this->offset(locoCount);
		if (address + WINSTON_STORAGE_LOCOSHED_STRIDE >= this->storage->capacity)
			return winston::Result::OutOfMemory;

		return winston::Result::OK;
	}

	const size_t LocomotiveShed::offset(const size_t count) const
	{
		return 2 + count * WINSTON_STORAGE_LOCOSHED_STRIDE;
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