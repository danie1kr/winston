#pragma once

#include "WinstonTypes.h"
#include "Util.h"

namespace winston
{
	class Locomotive : public Shared_Ptr<Locomotive>
	{
	public:
		Locomotive(const Address address, const std::string name);
		void light(bool on);
		bool light();
		bool forward();
		unsigned char speed();
		void drive(bool forward, unsigned char speed);
		void stop();
		void update(bool busy, bool forward, unsigned char speed, uint32_t functions);
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
	};
}

