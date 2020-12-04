#pragma once

#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
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
		
		Locomotive(const Callbacks callbacks, const Address address, const std::string name);
		inline void light(bool on);
		const bool light();
		const bool forward();
		const unsigned char speed();
		void drive(const bool forward, const unsigned char speed);
		void stop();
		void update(const bool busy, const bool forward, const unsigned char speed, const uint32_t functions);
		const Address& address();
		const std::string& name();
	private:

		struct Details
		{
			Address address = { 0 };
			std::string name = { "" };
			bool busy = { false };
			bool forward = { true };
			unsigned char speed = { 0 };
			uint32_t functions = { 0 };
		} details;

		const Callbacks callbacks;
	};
}

