#include "HAL.h"

namespace winston
{
	namespace hal
	{
		Socket::Socket() : state(State::NotConnected)
		{
		}

		const bool Socket::isConnected()
		{
			return this->state == State::Connected;
		}

		DebugSocket::DebugSocket(const Listener listener)
			: Socket(), Shared_Ptr<DebugSocket>(), listener(listener), buffer()
		{

		}

		const Result DebugSocket::send(const std::vector<unsigned char> data)
		{
			return this->listener(*this, data);
		}

		const Result DebugSocket::recv(std::vector<unsigned char>& data)
		{
			if (!this->buffer.empty())
			{
				const auto &packet = this->buffer.front();
				data.insert(data.begin(), packet.begin(), packet.end());
				this->buffer.pop();
			}
			return Result::OK;
		}

		void DebugSocket::addRecvPacket(const Packet data)
		{
			this->buffer.push(data);
		}

		StorageInterface::StorageInterface(const size_t maxSize)
			: Shared_Ptr<StorageInterface>(), maxSize(maxSize)
		{

		}

		DisplayUX::DisplayUX(const unsigned int width, const unsigned int height)
			: Shared_Ptr<DisplayUX>(), width(width), height(height)
		{

		}
	}
}
