#pragma once

#include "../libwinston/Winston.h"

template<typename T>
class IC74HC695 : public winston::SignalDevice<T>, public winston::Shared_Ptr<IC74HC695<T>>
{
	//static_assert(bits <= sizeof(T) * 8, "too many bits for T");
public:
	const size_t bits = 1;

	IC74HC695(typename winston::SendDevice<T>::Shared device)
		: winston::SignalDevice<T>(8, device), data((8 * bits / 8) / sizeof(T), 0)
	{
	}

	using winston::Shared_Ptr<IC74HC695<T>>::Shared;
	using winston::Shared_Ptr<IC74HC695<T>>::make;
private:

	const winston::Result updateInternal(winston::Signal::Shared signal)
	{
		for (auto& light : signal->lights())
			setPort(light.port, light.value);
		return this->flush();
	}

	virtual const winston::Result flushInternal()
	{
		return this->device->send(data);
	}

	void setPort(const size_t port, unsigned int value)
	{
		if (port >= this->ports)
			return;

#define SET_nth_BIT(data, bit, value) (data & (~(1 << bit))) | (value << i);
		this->data[0] = SET_nth_BIT(this->data[0], port, value ? 1 : 0);
#undef SET_nth_BIT
	}

	std::vector<T> data;
};

using IC74HC695_SignalDevice = IC74HC695<unsigned char>;