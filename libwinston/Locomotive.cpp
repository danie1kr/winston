#include "Locomotive.h"

namespace winston
{
	Locomotive::Locomotive(const Address address, const std::string name) :
		details{ .address = address, .name = name }
	{
	}

	void Locomotive::light(bool on)
	{
	}

	bool Locomotive::light()
	{
		return this->details.functions & 0b1;
	}
	
	bool Locomotive::forward()
	{
		return this->details.forward;
	}
	
	unsigned char Locomotive::speed()
	{
		return this->details.speed;
	}

	void Locomotive::drive(bool forward, unsigned char speed)
	{
	}

	void Locomotive::stop()
	{
		this->details.speed = 0;
	}

	void Locomotive::update(bool busy, bool forward, unsigned char speed, uint32_t functions)
	{
		this->details.busy = busy;
		this->details.forward = forward;
		this->details.speed = speed;
		this->details.functions = functions;
	}

	const Address& Locomotive::address()
	{
		return this->details.address;
	}

	const std::string& Locomotive::name()
	{
		return this->details.name;
	}
}