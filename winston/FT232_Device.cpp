#include "FT232_Device.h"

FT232_Device::FT232_GPIODigitalPinOutputDevice::FT232_GPIODigitalPinOutputDevice(FT232_Device& parent, const Pin pin, const State initial)
    : GPIODigitalPinOutputDevice(pin, initial), winston::Shared_Ptr<FT232_GPIODigitalPinOutputDevice>(), parent(parent)
{

}
void FT232_Device::FT232_GPIODigitalPinOutputDevice::set(const State value)
{
    this->parent.setGPIOPin(this->pin, value);
}

FT232_Device::FT232_Device(const Pin chipSelect, const unsigned int speed, SPIDataOrder order, SPIMode mode, const Pin clock, const Pin mosi, const Pin miso)
    : SPIDevice<unsigned char>(chipSelect, speed, order, mode, clock, mosi, miso), winston::GPIODevice(), initialized(false), ftHandle(nullptr), gpioState(0), gpioMode(0)
{
}

const winston::Result FT232_Device::init()
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
        winston::hal::fatal("chip select pin not in range 3-7");
    }

    channelConf.configOptions = configMode | configCS | SPI_CONFIG_OPTION_CS_ACTIVELOW;
    channelConf.Pin = 0x00000000;/*FinalVal-FinalDir-InitVal-InitDir (for dir 0=in, 1=out)*/

    this->gpioMode = 0b00000011;

    status = SPI_InitChannel(this->ftHandle, &channelConf);

    this->initialized = status == FT_OK;

    return status == FT_OK ? winston::Result::OK : winston::Result::ExternalHardwareFailed;
}
const winston::Result FT232_Device::send(const std::vector<DataType> data)
{
    if (this->skip)
        return winston::Result::OK;

    if (!this->initialized)
        return winston::Result::NotInitialized;

    FT_STATUS status = FT_OK;
    uint32 transfered = 0;
    uint8* dataPointer = (uint8*)&data.front();
    status = SPI_Write(this->ftHandle, dataPointer, (uint32)(data.size() * sizeof(DataType)), &transfered, SPI_TRANSFER_OPTIONS_SIZE_IN_BYTES |
        SPI_TRANSFER_OPTIONS_CHIPSELECT_ENABLE | SPI_TRANSFER_OPTIONS_CHIPSELECT_DISABLE);

    return status == FT_OK ? winston::Result::OK : winston::Result::ExternalHardwareFailed;
}

const winston::Result FT232_Device::setGPIOPin(const winston::GPIODigitalPinOutputDevice::Pin pin, const winston::GPIODigitalPinOutputDevice::State value)
{
    if (!this->initialized)
        return winston::Result::NotInitialized;

    FT_STATUS status = FT_OK;
    unsigned char shiftedPin = 1 << pin;
    if (this->gpioMode & shiftedPin)
    {
        if(value == winston::GPIODigitalPinOutputDevice::State::Low)
            this->gpioState &= ~shiftedPin;
        else
            this->gpioState |= shiftedPin;

        status = FT_WriteGPIO(this->ftHandle, this->gpioMode, this->gpioState);
        return status == FT_OK ? winston::Result::OK : winston::Result::ExternalHardwareFailed;
    }
    else
        return winston::Result::NotInitialized;
}

winston::GPIODigitalPinInputDevice::Shared FT232_Device::getInputPinDevice(winston::GPIODigitalPinInputDevice::Pin pin, winston::GPIODigitalPinInputDevice::Mode)
{
    return nullptr;
}

winston::GPIODigitalPinOutputDevice::Shared FT232_Device::getOutputPinDevice(winston::GPIODigitalPinOutputDevice::Pin pin)
{
    this->gpioMode |= (1 << pin);
    return FT232_GPIODigitalPinOutputDevice::make(*this, pin);
}