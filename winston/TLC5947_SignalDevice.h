#pragma once

#include "../libwinston/Winston.h"

template<typename T, unsigned int bits = 8 * sizeof(T)>
class TLC5947 : public winston::SignalDevice<T, bits>, public winston::Shared_Ptr<TLC5947<T, bits>>
{
	static_assert(bits <= sizeof(T) * 8, "too many bits for T");
public:
	TLC5947(const size_t devices, const size_t portsPerDevice, winston::SendDevice<T, bits>::Shared device)
		: winston::SignalDevice<T, bits>(devices, portsPerDevice, device), data(devices * portsPerDevice * bits / 8, 0)
	{

	}

	const winston::Result update(winston::Signal::Shared signal)
	{
	//	if (signal->port().device() >= this->portsPerDevice)
	//		return winston::Result::InternalError;

		for (auto& light : signal->lights())
			setPort(light.port.device(), light.port.port(), light.value);
		/*
		const unsigned int maximum = (1 << bits) - 1;
		if (signal->shows(winston::Signal::Aspect::Go))
		{
			setPort(signal->port().device(), signal->port().port(), 0);
			setPort(signal->port().device(), signal->port().port() + 1, maximum);
		}
		if (signal->shows(winston::Signal::Aspect::Halt))
		{
			setPort(signal->port().device(), signal->port().port(), maximum);
			setPort(signal->port().device(), signal->port().port() + 1, 0);
		}
		*/
		this->device->send(data);
		return winston::Result::OK;
	}

	using winston::Shared_Ptr<TLC5947<T, bits>>::Shared;
	using winston::Shared_Ptr<TLC5947<T, bits>>::make;
private:

	void setPort(const size_t dev, const size_t port, T value)
	{
		if (dev > this->devices || port >= this->portsPerDevice)
			return;
		size_t n = (bits * (dev * this->portsPerDevice + port)) / 8;
		unsigned char bytes[2] = { 0, 0 };
		if (port % 2 == 0)
		{
			// value = 0123456789AB
			// byte0 = 01234567
			// byte1 = 89ABXXXX
			// MSB done by SPI
			bytes[0] = value >> 4;
			bytes[1] = ((value & 0xF) << 4) | (this->data[n + 1] & 0xF);
		}
		else
		{
			// value = 0123456789AB
			// byte0 = XXXX0123
			// byte1 = 456789AB
			// MSB done by SPI
			bytes[0] = (this->data[n] & 0xF0) | (value >> 8);
			bytes[1] = value & 0xFF;
		}
		data[n] = bytes[0];
		data[n+1] = bytes[1];
	}

	std::vector<T> data;

};

using TLC5947_SignalDevice = TLC5947<unsigned int, 12>;