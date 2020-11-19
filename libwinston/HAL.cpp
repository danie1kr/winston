#include "HAL.h"

namespace winston
{
	namespace hal
	{
		UDPSocket::UDPSocket(const std::string ip, const unsigned short port) : state(State::NotConnected)
		{
		}

		const bool UDPSocket::isConnected()
		{
			return this->state == State::Connected;
		}
	}
}
