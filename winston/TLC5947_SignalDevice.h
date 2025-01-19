#pragma once

#include "../libwinston/Winston.h"

class TLC5947 : public winston::SignalDevice, public winston::Shared_Ptr<TLC5947>
{
	//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
public:
	static const size_t bits = 12;
#ifdef __GNUC__ 
	inline 
#endif
	static const unsigned int SPI_Clock = 1000000;
	#pragma message("TLC5947 Chaining: For multiple devices, use ring connection for clock (at least) or spi buffer/repeaters like 74lvc245 or TXU0304")
	// see https://forums.adafruit.com/viewtopic.php?t=58367&start=15

	TLC5947(const winston::Id id, const size_t ports, typename winston::SendDevice<unsigned char>::Shared device, typename winston::GPIODigitalPinOutputDevice::Shared pinOff);
	virtual ~TLC5947();

	using winston::Shared_Ptr<TLC5947>::Shared;
	using winston::Shared_Ptr<TLC5947>::make;
private:

	const winston::Result updateInternal(const winston::Signal& signal);
	virtual const winston::Result flushInternal();
	void setPort(const size_t port, unsigned int value);

	winston::SendDevice<unsigned char>::Shared device;
	std::vector<unsigned char> data;
	winston::GPIODigitalPinOutputDevice::Shared pinOff;
};