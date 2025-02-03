#include "HAL.h"
#include "Util.h"

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
#ifdef WINSTON_PLATFORM_WIN_x64
		DebugSocket::DebugSocket(const Listener listener)
			: Socket(), Shared_Ptr<DebugSocket>(), buffer(), listener(listener)
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
#endif
		StorageInterface::StorageInterface(const size_t capacity)
			: Shared_Ptr<StorageInterface>(), capacity(capacity)
		{

		}

		const Result StorageInterface::readUint32(const size_t address, uint32_t& content)
		{
			uint8_t f0; if (const auto status = this->read(address + 0, f0); status != Result::OK) return status;
			uint8_t f1; if (const auto status = this->read(address + 1, f1); status != Result::OK) return status;
			uint8_t f2; if (const auto status = this->read(address + 2, f2); status != Result::OK) return status;
			uint8_t f3; if (const auto status = this->read(address + 3, f3); status != Result::OK) return status;
			content = ((uint32_t)f0 << 0) | ((uint32_t)f1 << 8) | ((uint32_t)f2 << 16) | ((uint32_t)f3 << 24);
			return Result::OK;
		}

		const Result StorageInterface::readUint16(const size_t address, uint16_t& content)
		{
			uint8_t f0; if (const auto status = this->read(address + 0, f0); status != Result::OK) return status;
			uint8_t f1; if (const auto status = this->read(address + 1, f1); status != Result::OK) return status;
			content = ((uint32_t)f0 << 0) | ((uint32_t)f1 << 8);
			return Result::OK;
		}

		const Result StorageInterface::readFloat(const size_t address, float& content)
		{
			uint8_t f0; if (const auto status = this->read(address + 0, f0); status != Result::OK) return status;
			uint8_t f1; if (const auto status = this->read(address + 1, f1); status != Result::OK) return status;
			uint8_t f2; if (const auto status = this->read(address + 2, f2); status != Result::OK) return status;
			uint8_t f3; if (const auto status = this->read(address + 3, f3); status != Result::OK) return status;
			uint32_t f = ((uint32_t)f0 << 0) | ((uint32_t)f1 << 8) | ((uint32_t)f2 << 16) | ((uint32_t)f3 << 24);

			winston::memcpy_s(&content, sizeof(content), &f, sizeof(f));

			return Result::OK;
		}

		const Result StorageInterface::writeUint32(const size_t address, const uint32_t content)
		{
			uint8_t d;
			d = (content >> 0) & 0xFF; if (const auto status = this->write(address + 0, d); status != Result::OK) return status;
			d = (content >> 8) & 0xFF; if (const auto status = this->write(address + 1, d); status != Result::OK) return status;
			d = (content >> 16) & 0xFF; if (const auto status = this->write(address + 2, d); status != Result::OK) return status;
			d = (content >> 24) & 0xFF; if (const auto status = this->write(address + 3, d); status != Result::OK) return status;
			return Result::OK;
		}

		const Result StorageInterface::writeUint16(const size_t address, const uint16_t content)
		{
			uint8_t d;
			d = (content >> 0) & 0xFF; if (const auto status = this->write(address + 0, d); status != Result::OK) return status;
			d = (content >> 8) & 0xFF; if (const auto status = this->write(address + 1, d); status != Result::OK) return status;
			return Result::OK;
		}

		const Result StorageInterface::writeFloat(const size_t address, const float content)
		{
			uint32_t f;
			winston::memcpy_s(&f, sizeof(f), &content, sizeof(content));
			uint8_t d;
			d = (f >> 0) & 0xFF; if (const auto status = this->write(address + 0, d); status != Result::OK) return status;
			d = (f >> 8) & 0xFF; if (const auto status = this->write(address + 1, d); status != Result::OK) return status;
			d = (f >> 16) & 0xFF; if (const auto status = this->write(address + 2, d); status != Result::OK) return status;
			d = (f >> 24) & 0xFF; if (const auto status = this->write(address + 3, d); status != Result::OK) return status;
			return Result::OK;
		}

		DisplayUX::DisplayUX(const unsigned int width, const unsigned int height)
			: Shared_Ptr<DisplayUX>(), width(width), height(height)
		{

		}
	}
}
