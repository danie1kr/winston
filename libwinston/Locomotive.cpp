
#include "Locomotive.h"
#include "Log.h"
#include "HAL.h"

namespace winston
{
	Position::Position(Track::Shared track, const Track::Connection reference, const Distance distance)
		: track(track), reference(reference), distance(distance)
	{

	}

	Position::Transit Position::drive(const Distance distance)
	{
		this->distance += distance;
		auto block = this->track->block();
		if (this->distance >= 0 && this->distance <= (int)this->track->length())
			return Transit::Stay;
		else
		{
			// negative = leave at reference, else other direction connection
			auto connection = this->distance < 0 ? this->reference : this->track->otherConnection(this->reference);
			this->distance = abs(this->distance);
			auto current = this->track;
			while (true)
			{
				auto onto = current;
				if (!track->traverse(connection, onto, true))
				{
					logger.err(build("cannot traverse during Position::drive: ", current->name(), " leaving on ", Track::ConnectionToString(connection)));
					return Transit::TraversalError;
				}

				if (this->distance < (int)onto->length())
					break;
				else
					this->distance -= onto->length();

				connection = onto->whereConnects(current);
				connection = onto->otherConnection(connection);
				current = onto;
			}
			this->track = current;
			return this->track->block() == block ? Transit::CrossTrack : Transit::CrossBlock;
		}
	}

	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const Position start, const std::string name, const NFCAddress nfcAddress) :
		callbacks(callbacks), details{ address, nfcAddress, start, hal::now(), name, false, true, 0, 0 }, speedMap()
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
		timeOnTour = this->details.lastPositionUpdate - now;
		this->details.lastPositionUpdate = now;
		this->details.position.drive(this->speedMap.speed(this->details.speed) * inMilliseconds(timeOnTour));
		return this->position();
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
		this->map.insert_or_assign(throttle, speed);
	}
	const Locomotive::SpeedMap::Speed Locomotive::SpeedMap::speed(const Throttle throttle) const
	{
		auto lookup = this->map.find(throttle);
		if (lookup == this->map.end())
		{
			auto lower = this->map.lower_bound(throttle);
			auto upper = this->map.upper_bound(throttle);

			float frac = (lower->first - throttle) / (upper->first - lower->first);
			
			// linear
			return this->lerp(lower->second, upper->second, frac);
		}
		else
			return lookup->second;
	}

	const Locomotive::SpeedMap::Speed Locomotive::SpeedMap::lerp(Locomotive::SpeedMap::Speed lower, Locomotive::SpeedMap::Speed upper, const float frac)
	{
		return (Speed)(frac * upper + (1.0f - frac) * lower);
	}
}