#pragma once
#include "../libwinston/Signal.h"
#include "../libwinston/HAL.h"

#include<windows.h>
#include "../external/ftdi-232/ftd2xx.h"
#include "../external/ftdi-232/libMPSSE_spi.h"

class FT232_Device : public winston::hal::SPIDevice<unsigned char>, public winston::GPIODevice, public winston::Shared_Ptr<FT232_Device>
{
private:
	class FT232_GPIODigitalPinOutputDevice : public winston::GPIODigitalPinOutputDevice, public winston::Shared_Ptr<FT232_GPIODigitalPinOutputDevice>
	{
	public:
		FT232_GPIODigitalPinOutputDevice(FT232_Device& parent, const Pin pin, const State initial = State::Low);
		void set(const State value);
		using winston::Shared_Ptr<FT232_GPIODigitalPinOutputDevice>::Shared;
		using winston::Shared_Ptr<FT232_GPIODigitalPinOutputDevice>::make;
	private:
		FT232_Device& parent;
	};

public:
	FT232_Device(const Pin chipSelect, const unsigned int speed, SPIDataOrder order = SPIDataOrder::MSBFirst, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0);

	using winston::Shared_Ptr<FT232_Device>::Shared;
	using winston::Shared_Ptr<FT232_Device>::make;

	const winston::Result init();
	const winston::Result send(const std::vector<DataType> data);

	winston::GPIODigitalPinInputDevice::Shared getInputPinDevice(winston::GPIODigitalPinInputDevice::Pin pin, winston::GPIODigitalPinInputDevice::Mode);
	winston::GPIODigitalPinOutputDevice::Shared getOutputPinDevice(winston::GPIODigitalPinOutputDevice::Pin pin);
private:
	const winston::Result setGPIOPin(const winston::GPIODigitalPinOutputDevice::Pin pin, const winston::GPIODigitalPinOutputDevice::State value);

	bool initialized;
	FT_HANDLE ftHandle;
	// only Bank B for now
	unsigned char gpioState;
	unsigned char gpioMode;
};

using SignalInterfaceDevice = FT232_Device;