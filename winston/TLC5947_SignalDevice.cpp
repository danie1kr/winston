#pragma once

#include "TLC5947_SignalDevice.h"

TLC5947::TLC5947(const size_t ports, typename winston::SendDevice<unsigned char>::Shared device, typename winston::GPIODigitalPinOutputDevice::Shared pinOff)
	: winston::SignalDevice(ports), device(device), data((ports * bits / 8) / sizeof(unsigned char), 0), pinOff(pinOff)
{
	pinOff->set(winston::GPIOPinDevice::State::High);
}

TLC5947::~TLC5947()
{
}

const winston::Result TLC5947::updateInternal(const winston::Signal& signal)
{
	for (auto& light : signal.lights())
		setPort(light.port, light.value);
	return this->flush();
}

const winston::Result TLC5947::flushInternal()
{
	pinOff->set(winston::GPIOPinDevice::State::Low);
	return this->device->send(data);
}

void TLC5947::setPort(const size_t port, unsigned int value)
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