#pragma once

#include "../libwinston/Winston.h"

template<typename T>
class TLC5947 : public winston::SignalDevice<T>, public winston::Shared_Ptr<TLC5947<T>>
{
	//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
public:
	const size_t bits = 12;

	TLC5947(const size_t devices, const size_t portsPerDevice, typename winston::SendDevice<T>::Shared device, typename winston::GPIODigitalPinOutputDevice::Shared pinOff)
		: winston::SignalDevice<T>(devices, portsPerDevice, device), data((devices * portsPerDevice * bits / 8) / sizeof(T), 0), pinOff(pinOff)
	{
		pinOff->set(winston::GPIOPinDevice::State::High);
	}

	using winston::Shared_Ptr<TLC5947<T>>::Shared;
	using winston::Shared_Ptr<TLC5947<T>>::make;
private:

	const winston::Result updateInternal(winston::Signal::Shared signal)
	{
		for (auto& light : signal->lights())
			setPort(light.port.device(), light.port.port(), light.value);
		
		winston::logger.info(winston::build("Before update/flush", (int)data[1], " ", (int)data[data.size() - 2]));
		winston::Result r = this->flush();
		winston::logger.info(winston::build("After update/flush", (int)data[1], " ", (int)data[data.size() - 2]));
		return r;
	}

	virtual const winston::Result flushInternal()
	{
		pinOff->set(winston::GPIOPinDevice::State::Low);
		winston::logger.info(winston::build("Before flush/send data", (int)data[1], " ", (int)data[data.size()-2]));
		std::vector<T> updateable(this->data);
		winston::logger.info(winston::build("Before flush/send upd", (int)updateable[1], " ", (int)updateable[updateable.size() - 2]));
		winston::Result r = this->device->send(updateable);
		winston::logger.info(winston::build("After flush/send upd", (int)updateable[1], " ", (int)updateable[updateable.size() - 2]));
		winston::logger.info(winston::build("After flush/send", (int)data[1], " ", (int)data[data.size() - 2]));
		return r;
	}

	void setPort(const size_t dev, const size_t port, unsigned int value)
	{
		if (dev >= this->devices || port >= this->portsPerDevice)
			return;
		//size_t n = (bits * (dev * this->portsPerDevice + port)) / 8;
		//size_t slots = (bits * (this->devices * this->portsPerDevice)) / 8;
		//size_t n = slots - 1 - (bits * (dev * this->portsPerDevice + port)) / 8 - 1;
		//size_t n = (bits * ((this->devices - 1 - dev) * this->portsPerDevice + (this->portsPerDevice - 1 - port))) / 8;
		
		size_t slots = this->devices * this->portsPerDevice;
		size_t n = (bits * (slots - 1 - ((dev) * this->portsPerDevice + (port)))) / 8;
		unsigned char bytes[2] = { 0, 0 };
		if (port % 2 != 0)
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
		winston::logger.info(winston::build("Setting port ", port, " to ", value, " idx: ", n, " before: ", (int)data[n], " ", (int)data[n+1], " after : ", (int)bytes[0], " ", (int)bytes[1]));
		data[n] = bytes[0];
		data[n+1] = bytes[1];
	}

	std::vector<T> data;

	winston::GPIODigitalPinOutputDevice::Shared pinOff;
};

using TLC5947_SignalDevice = TLC5947<unsigned char>;