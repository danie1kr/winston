#pragma once

#include <map>
#include "WinstonTypes.h"
#include "Track.h"

namespace winston
{
	class Position
	{
	public:
		enum class Transit
		{
			Stay		= 0,
			CrossTrack	= 1,
			CrossBlock	= 2,
			TraversalError = 3
		};

		Position(Track::Shared track, const Track::Connection reference, const Distance distance);
		Transit drive(const Distance distance);
	private:
		// the track we are on
		Track::Shared track;

		// the connection we use as reference for the distance
		Track::Connection reference;
		Distance distance;
	};

	// https://www.wolframalpha.com/input/?i=solve+s+%3D+%28v1%C2%B2-v0%C2%B2%29%2F2a+%2B+v1%28T+-+%28v1-v0%29%2Fa%29+for+a
	class Locomotive : public Shared_Ptr<Locomotive>
	{
	public:
		struct Callbacks
		{
			using DriveCallback = std::function<void(const Address address, const unsigned char speed, const bool forward)>;
			DriveCallback drive;

			using FunctionsCallback = std::function<void(const Address address, const uint32_t functions)>;
			FunctionsCallback functions;
		};
		
		Locomotive(const Callbacks callbacks, const Address address, const Position start, const std::string name, const NFCAddress nfcAddress);
		inline void light(bool on);
		const bool light();
		const bool forward();
		const unsigned char speed();
		void drive(const bool forward, const unsigned char speed);
		const Position& moved(Duration& timeOnTour);
		const Position& position();
		void stop();
		void update(const bool busy, const bool forward, const unsigned char speed, const uint32_t functions);
		const Address& address() const;
		const NFCAddress& nfcAddress() const;
		const std::string& name();
	private:

		class SpeedMap
		{
		public:
			using Throttle = unsigned char;
			using Speed = unsigned int;
			const Speed speed(const Throttle throttle) const;
			void learn(const Throttle throttle, const Speed speed);
			std::map<Throttle, Speed> map;
		private:
			static const Speed lerp(const Speed lower, const Speed upper, const float frac);
		};

		const Callbacks callbacks;
		struct Details
		{
			Address address = { 0 };
			NFCAddress nfcAddress = { 0 };
			Position position;
			TimePoint lastPositionUpdate;
			std::string name = { "" };
			bool busy = { false };
			bool forward = { true };
			unsigned char speed = { 0 };
			uint32_t functions = { 0 };
		} details;

		SpeedMap speedMap;
	};
}

