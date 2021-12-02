#include "FT232_SPIDevice.h"

FT232_SPIDevice::FT232_SPIDevice(const Pin chipSelect, const unsigned int speed, SPIDataOrder order, SPIMode mode, const Pin clock, const Pin mosi, const Pin miso)
    : SPIDevice<unsigned int, 12>(chipSelect, speed, order, mode, clock, mosi, miso)
{
}

const winston::Result FT232_SPIDevice::init()
{
    FT_STATUS status = FT_OK;
    Init_libMPSSE();
    uint32 channels = 0;
    status = SPI_GetNumChannels(&channels);

    if (status != FT_OK || channels == 0)
        return winston::Result::ExternalHardwareFailed;

    // always open first channel
    status = SPI_OpenChannel(0, &ftHandle);

    ChannelConfig channelConf = { 0 };
    channelConf.ClockRate = this->speed;
    channelConf.LatencyTimer = 255;

    unsigned int configMode = 0;
    switch (this->mode)
    {
    case SPIMode::SPI_0: configMode = SPI_CONFIG_OPTION_MODE0; break;
    case SPIMode::SPI_1: configMode = SPI_CONFIG_OPTION_MODE1; break;
    case SPIMode::SPI_2: configMode = SPI_CONFIG_OPTION_MODE2; break;
    case SPIMode::SPI_3: configMode = SPI_CONFIG_OPTION_MODE3; break;
    }

    unsigned int configCS = 0;
    switch (this->chipSelect)
    {
    case 3: configCS = SPI_CONFIG_OPTION_CS_DBUS3; break;
    case 4: configCS = SPI_CONFIG_OPTION_CS_DBUS4; break;
    case 5: configCS = SPI_CONFIG_OPTION_CS_DBUS5; break;
    case 6: configCS = SPI_CONFIG_OPTION_CS_DBUS6; break;
    case 7: configCS = SPI_CONFIG_OPTION_CS_DBUS7; break;
    default:
        winston::hal::fatal("chip select not in range 3-7");
    }

    channelConf.configOptions = configMode | configCS;// | SPI_CONFIG_OPTION_CS_ACTIVELOW;
    channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/
    status = SPI_InitChannel(ftHandle, &channelConf);

    return winston::Result::OK;
}
const winston::Result FT232_SPIDevice::send(const std::span<DataType> data)
{
    if (this->skip)
        return winston::Result::OK;

    FT_STATUS status = FT_OK;
    uint32 transfered = 0;
    uint8* dataPointer = (uint8*)&data.front();
    status = SPI_Write(ftHandle, dataPointer, (uint32)( data.size() * sizeof(DataType)), &transfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES |
        SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);
    return status == FT_OK ? winston::Result::OK : winston::Result::ExternalHardwareFailed;
}