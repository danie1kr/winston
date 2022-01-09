#pragma once
#include "../libwinston/Signal.h"
#include "../libwinston/HAL.h"
#include "../libwinston/span.hpp"

#include<windows.h>
#include "../external/ftdi-232/ftd2xx.h"
#include "../external/ftdi-232/libMPSSE_spi.h"

class FT232_SPIDevice : public winston::hal::SPIDevice<unsigned char>, public winston::Shared_Ptr<FT232_SPIDevice>
{
public:
	FT232_SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order = SPIDataOrder::MSBFirst, SPIMode mode = SPIMode::SPI_0, const Pin clock = 0, const Pin mosi = 0, const Pin miso = 0);

	using winston::Shared_Ptr<FT232_SPIDevice>::Shared;
	using winston::Shared_Ptr<FT232_SPIDevice>::make;

	const winston::Result init();
	const winston::Result send(const std::span<DataType> data);
private:
	FT_HANDLE ftHandle;
};