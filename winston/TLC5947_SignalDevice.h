#pragma once

#include "../libwinston/Winston.h"

template<typename T>
class TLC5947 : public winston::SignalDevice<T>, public winston::Shared_Ptr<TLC5947<T>>
{
	//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
public:
	static const size_t bits = 12;
	static const unsigned int SPI_Clock = 20000;
	#pragma message("TLC5947 Chaining: For more than 3 devices, use a very low clock (<10khz) or spi buffer/repeaters like 74lvc245 or TXU0304")
	// see https://forums.adafruit.com/viewtopic.php?t=58367&start=15

	TLC5947(const size_t ports, typename winston::SendDevice<T>::Shared device, typename winston::GPIODigitalPinOutputDevice::Shared pinOff)
		: winston::SignalDevice<T>(ports, device), data((ports * bits / 8) / sizeof(T), 0), pinOff(pinOff)
	{
		pinOff->set(winston::GPIOPinDevice::State::High);
	}

	using winston::Shared_Ptr<TLC5947<T>>::Shared;
	using winston::Shared_Ptr<TLC5947<T>>::make;
private:

	const winston::Result updateInternal(winston::Signal::Shared signal)
	{
		for (auto& light : signal->lights())
			setPort(light.port, light.value);
		return this->flush();
	}

	virtual const winston::Result flushInternal()
	{
		pinOff->set(winston::GPIOPinDevice::State::Low);
		//std::vector<T> updateable(this->data);
		return this->device->send(data);
	}

	void setPort(const size_t port, unsigned int value)
	{
		if (port >= this->ports)
			return;
		
		size_t n = (bits * (this->ports - 1 - port)) / 8;
		unsigned char bytes[2] = { 0, 0 };
		if (port % 2 != 0)	// even
		{
			// value = 0123456789AB
			// byte0 = 01234567
			// byte1 = 89ABXXXX
			// MSB done by SPI
			// swap 0 and 1?
			bytes[0] = value >> 4;
			bytes[1] = ((value & 0xF) << 4) | (this->data[n + 1] & 0xF);
		}
		else				// odd
		{
			// value = 0123456789AB
			// byte0 = XXXX0123
			// byte1 = 456789AB
			// MSB done by SPI
			bytes[0] = (this->data[n] & 0xF0) | ((value >> 8) & 0x0F);
			bytes[1] = value & 0xFF;
		}
		data[n] = bytes[0];
		data[n+1] = bytes[1];
	}

	std::vector<T> data;

	winston::GPIODigitalPinOutputDevice::Shared pinOff;
};

using TLC5947_SignalDevice = TLC5947<unsigned char>;