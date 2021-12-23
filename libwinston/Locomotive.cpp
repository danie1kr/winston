#include "Locomotive.h"

namespace winston
{
	Locomotive::Locomotive(const Callbacks callbacks, const Address address, const std::string name) :
		callbacks(callbacks), details{ address, name, false, true, 0, 0 }
	{
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

	const Address& Locomotive::address() const
	{
		return this->details.address;
	}

	const std::string& Locomotive::name()
	{
		return this->details.name;
	}
}