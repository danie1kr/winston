#include "HAL.h"

namespace winston
{
	namespace hal
	{
		Socket::Socket(const std::string ip, const unsigned short port) : state(State::NotConnected)
		{
		}

		const bool Socket::isConnected()
		{
			return this->state == State::Connected;
		}

		StorageInterface::StorageInterface(const size_t maxSize)
			: Shared_Ptr<StorageInterface>(), maxSize(maxSize)
		{

		}
	}
}
