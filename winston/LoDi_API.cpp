#include "../libwinston/Log.h"
#include "LoDi_API.h"

LoDi::LoDi(winston::hal::Socket::Shared socket)
	: winston::Shared_Ptr<LoDi>(), winston::Looper(), packetParser(socket)
{

}

const winston::Result LoDi::discover()
{
	return winston::Result::OK;
}

LoDi::S88Commander::Shared LoDi::createS88Commander()
{
	return LoDi::S88Commander::make(this->packetParser, "LoDi Commander");
}

const winston::Result LoDi::loop()
{
	return this->packetParser.loop();
}

LoDi::PacketParser::PacketParser(winston::hal::Socket::Shared socket)
	: Looper(), packetNumber(0), socket(socket), buffer()
{

}

const winston::Result LoDi::PacketParser::loop()
{
	std::vector<unsigned char> data;
	auto result = this->socket->recv(data);
	if (data.size() > 0)
		this->buffer.insert(this->buffer.end(), data.begin(), data.end());

	this->processBuffer();

	return winston::Result::OK;
}

const winston::Result LoDi::PacketParser::processBuffer()
{
	if (this->buffer.size() >= 5)
	{
		unsigned int packetSize = (((unsigned int)this->buffer[0]) << 8) + this->buffer[1];
		if (this->buffer.size() >= packetSize + 2)
		{
			std::vector<uint8_t> packet;
			packet.insert(packet.begin(), this->buffer.begin() + 2, this->buffer.begin() + 2 + packetSize);
			this->processPacket(packet);
			
			// remove processed packet
			for(size_t i = 0; i < packetSize + 2; ++i)
				this->buffer.pop_front();
		}
	}
	return winston::Result::OK;
}

const winston::Result LoDi::PacketParser::processPacket(const Payload payload)
{
	const auto type = (LoDi::API::PacketType)payload[0];
	const auto packetNumber = payload[2];
	Payload data;
	data.insert(data.begin(), payload.begin() + 3, payload.end());
	switch (type)
	{
	case LoDi::API::PacketType::EVT:
	{
		const auto event = (LoDi::API::Event)payload[1];
		return this->processEvent(event, data);
	}
	case LoDi::API::PacketType::ACK:
	{
		const auto command = (LoDi::API::Command)payload[1];
		if (this->expectedResponse.find(packetNumber) != this->expectedResponse.end())
		{
			{
				auto& eR = this->expectedResponse[packetNumber];
				if (eR.command == command)
				{
					if (eR.callback)
					{
						if (eR.callback(data) == winston::Result::OK)
						{

						}
						else
						{
							winston::logger.err("LoDi response callback failed: Type=", winston::hex((unsigned int)type), " Command=", winston::hex((unsigned int)command), " Number=", packetNumber);
						}
					}
				}
			}
			this->expectedResponse.erase(packetNumber);
		}
		else
			return winston::Result::NotFound;
		break;
	}
	case LoDi::API::PacketType::BUSY:
	{
		const auto command = (LoDi::API::Command)payload[1];
		if (this->expectedResponse.find(packetNumber) != this->expectedResponse.end())
		{
			{
				auto& eR = this->expectedResponse[packetNumber];
				this->sendAgain(eR);
			}

			if (this->expectedResponse[packetNumber].sentCount > 10)
			{
				winston::logger.err("LoDi erasing packet for >10 retries Command=", winston::hex((unsigned int)command), " Number=", packetNumber);
				this->expectedResponse.erase(packetNumber);
			}
		}
		break;
	}
	case LoDi::API::PacketType::NACK:
	{
		const auto command = (LoDi::API::Command)payload[1];
		if (this->expectedResponse.find(packetNumber) != this->expectedResponse.end())
			this->expectedResponse.erase(packetNumber);
		winston::logger.err("LoDi NACK for Command = ", winston::hex((unsigned int)command), " Number = ", packetNumber);
		break;
	}
	default:
	{
		winston::logger.err("LoDi unknown packet type: ", winston::hex((unsigned int)type));
		return winston::Result::NotFound;
	}
	}
	return winston::Result::OK;
}

const winston::Result LoDi::PacketParser::processEvent(const API::Event event, const Payload payload)
{
	switch (event)
	{
	case API::Event::S88LokAddrEvent:
	{
		size_t idx = 0;
		uint8_t count = payload[idx++];
		for (size_t i = 0; i < count; ++i)
		{
			uint8_t address = payload[idx++];
			uint8_t channel = payload[idx++];
			uint8_t high = payload[idx++];
			uint8_t low = payload[idx++];
			uint8_t status = payload[idx++];

			this->detectorDevice->change(address * 8 + channel, (high << 8) + low, status == 1 ? winston::Detector::Change::Entered : winston::Detector::Change::Left);
		}
		break;
	}
	case API::Event::S88MelderEvent:
	{
		size_t idx = 0;
		uint8_t count = payload[idx++];
		for (size_t i = 0; i < count; ++i)
		{
			uint8_t address = payload[idx++];
			uint8_t channel = payload[idx++];
			bool empty = (payload[idx++] == 0) ? true : false;
			this->detectorDevice->occupied(address * 8 + channel, empty ? winston::Detector::Change::Left : winston::Detector::Change::Entered);
		}
		break;
	}
	case API::Event::S88RailcomResponseEvent:
	{

		break;
	}
	case API::Event::S88RawEvent:
	{
		break;
	}
	default:
	{
		winston::logger.err("LoDi unknown event type: ", winston::hex((unsigned int)event));
		return winston::Result::NotFound;
	}
	}

	return winston::Result::OK;
}

const winston::Result LoDi::PacketParser::send(const LoDi::API::Command command, const Payload payload, PacketCallback callback)
{
	uint8_t packetNumber = this->nextPacketNumber();
	std::vector<uint8_t> data;
	size_t size = 5 + payload.size();
	data.push_back((uint8_t)(size >> 8));
	data.push_back((uint8_t)(size & 0xFF));
	data.push_back((uint8_t)LoDi::API::PacketType::REQ);
	data.push_back((uint8_t)command);
	data.push_back(packetNumber);
	data.insert(data.end(), payload.begin(), payload.end());

	PacketAndCallback pAC;
	pAC.command = command;
	pAC.callback = callback;
	pAC.data = data;
	pAC.sentCount = 0;
	
	this->sendAgain(pAC);

	this->expectedResponse[packetNumber] = pAC;
	return winston::Result::OK;
}

void LoDi::PacketParser::setDetectorDevice(winston::DetectorDevice::Shared detectorDevice)
{
	this->detectorDevice = detectorDevice;
}

const winston::Result LoDi::PacketParser::sendAgain(PacketAndCallback& packetAndCallback)
{
	packetAndCallback.sentAt = winston::hal::now();
	packetAndCallback.sentCount++;
	this->socket->send(packetAndCallback.data);

	return winston::Result::OK;
}

const uint8_t LoDi::PacketParser::nextPacketNumber()
{
	this->packetNumber = (this->packetNumber % 250) + 1;
	return this->packetNumber;
}

LoDi::S88Commander::S88Commander(PacketParser& packetParser, const std::string name)
	: winston::Shared_Ptr<S88Commander>(), winston::DetectorDevice(name), _state(State::Unknown), initializedComponents((unsigned int)Initialized::Uninitialized), packetParser(packetParser)
{
}

const winston::Result LoDi::S88Commander::init(PortSegmentMap portSegmentMap, Callbacks callbacks)
{
	this->_state = State::Initializing;
	this->packetParser.setDetectorDevice(this->DetectorDevice::shared_from_this());

	if (const auto result = this->initInternal(callbacks); result != winston::Result::OK)
		return result;

	for (auto& portSegment : portSegmentMap)
		this->ports.insert(std::make_pair(portSegment.first, winston::Detector::make(winston::build(this->name, " ", portSegment.first), portSegment.second, callbacks.change)));

	this->getVersion();
	this->deviceConfigGet();
	this->s88EventsActivate(true);
	
	return winston::Result::OK;
}

const LoDi::S88Commander::State LoDi::S88Commander::state()
{
	if(this->initializedComponents & (unsigned int)Initialized::Finished)
		this->_state = State::Ready;
	return this->_state;
}

const bool LoDi::S88Commander::isReady()
{
	return this->state() == State::Ready;
}

const winston::Result LoDi::S88Commander::getVersion()
{
	Payload payload;
	return this->packetParser.send(LoDi::API::Command::GetVersion, payload,
		[this](const Payload payload) -> const winston::Result
		{
			if (payload.size() == 4)
			{
				this->initializedComponents |= (unsigned int)Initialized::Version;
				if (payload[0] == 0x0A)
				{
					winston::logger.info("LoDi S88 Commander version: ", payload[1], ".", payload[2], ".", payload[3]);
					return winston::Result::OK;
				}
				else
				{
					winston::logger.err("LoDi expected Device type 0x0A but got: ", winston::hex(payload[0]));
					return winston::Result::ExternalHardwareFailed;
				}
			}
			else
			return winston::Result::InvalidParameter;
		}
	);
}

const winston::Result LoDi::S88Commander::deviceConfigGet()
{
	Payload payload;
	return this->packetParser.send(LoDi::API::Command::DeviceConfigGet, payload,
		[this](const Payload payload) -> const winston::Result
		{
			if (payload.size() == 16)
			{
				this->initializedComponents |= (unsigned int)Initialized::DeviceConfig;
				winston::logger.info("LoDi S88 Commander Device Config Bus0: ", payload[0], ", Bus1: ", payload[1]);

				return winston::Result::OK;
			}
			else
				return winston::Result::InvalidParameter;
		}
	);
}

const winston::Result LoDi::S88Commander::s88BusModulesGet()
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88ModulNameGet()
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88ChannelNamesGet()
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88MelderGet()
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88LokAddrGet(const uint8_t module)
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88CurrentLevelsGet()
{
	return winston::Result::NotImplemented;
}

const winston::Result LoDi::S88Commander::s88EventsActivate(const bool activate)
{
	Payload payload;
	payload.push_back(activate ? 1 : 0);
	return this->packetParser.send(LoDi::API::Command::S88EventsActivate, payload,
		[this](const Payload payload) -> const winston::Result
		{
			// no payload expected
			this->initializedComponents |= (unsigned int)Initialized::EventsActive;
			winston::logger.info("LoDi S88 Commander Events active");
			return payload.size() == 0 ? winston::Result::OK : winston::Result::InvalidParameter;
		}
	);
}

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
winston::hal::DebugSocket::Shared LoDi::createLoDiDebugSocket()
{
	return winston::hal::DebugSocket::make([](winston::hal::DebugSocket& socket, const std::vector<unsigned char> data) -> const winston::Result
		{
			const auto type = (LoDi::API::PacketType)data[2];
			const auto command = (LoDi::API::Command)data[3];
			const uint8_t number = data[4];
			if (type == LoDi::API::PacketType::REQ && command == LoDi::API::Command::GetVersion)
			{
				winston::hal::DebugSocket::Packet packet;
				packet.push_back(0);     // size
				packet.push_back(7);     // size
				packet.push_back((unsigned char)LoDi::API::PacketType::ACK);  // ACK
				packet.push_back((unsigned char)command);  // GetVersion
				packet.push_back(number);// Packet number
				packet.push_back(0x0A);  // device type
				packet.push_back(24);    // major
				packet.push_back(0);     // minor
				packet.push_back(1);     // patch
				packet[1] = (unsigned char)(packet.size() - 2);// update size
				socket.addRecvPacket(packet);
			}
			else if (type == LoDi::API::PacketType::REQ && command == LoDi::API::Command::DeviceConfigGet)
			{
				winston::hal::DebugSocket::Packet packet;
				packet.push_back(0);     // size
				packet.push_back(19);     // size
				packet.push_back((unsigned char)LoDi::API::PacketType::ACK);  // ACK
				packet.push_back((unsigned char)command);  // GetVersion
				packet.push_back(number);// Packet number
				packet.push_back(2);  // length bus 1
				packet.push_back(0);  // length bus 2
				packet.push_back(0);  // speed bus 1
				packet.push_back(0);  // speed bus 1
				packet.push_back(0);  // start delay bus 1
				packet.push_back(0);  // start delay bus 1
				packet.push_back(10);  // stop delay bus 1
				packet.push_back(10);  // stop delay bus 1
				packet.push_back(0);  // disconnect busses
				packet.push_back(1);  // µCon compat
				packet.push_back(1);  // use addresses
				packet.push_back(0);  // rain mode
				packet.push_back(1);  // railcom speed
				packet.push_back(1);  // railcom qos
				packet.push_back(10000 >> 8);  // baudrate high
				packet.push_back(0);  // baudrate low
				packet[1] = (unsigned char)(packet.size() - 2);// update size
				socket.addRecvPacket(packet);
			}
			else if (type == LoDi::API::PacketType::REQ && command == LoDi::API::Command::S88EventsActivate)
			{
				winston::hal::DebugSocket::Packet packet;
				packet.push_back(0);     // size
				packet.push_back(3);     // size
				packet.push_back((unsigned char)LoDi::API::PacketType::ACK);  // ACK
				packet.push_back((unsigned char)command);  // GetVersion
				packet.push_back(number);// Packet number
				packet[1] = (unsigned char)(packet.size() - 2);// update size
				socket.addRecvPacket(packet);
			}
			else
			{
				winston::logger.info("DebugSocket: unanswered type: ", winston::hex((unsigned char)type), " and command: ", winston::hex((unsigned char)command));
			}
			return winston::Result::OK;
		});
}
#endif