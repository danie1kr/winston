#pragma once

#include <map>
#include <functional>
#include "../libwinston/HAL.h"
#include "../libwinston/WinstonTypes.h"

#include "../libwinston/Detector.h"

// see https://lokstoredigital.jimdoweb.com/service/ger%C3%A4te-api/allgemeines/
// and https://www.lokstoredigital.de/hardware/melden/lodi-s88-commander/

class LoDi : public winston::Shared_Ptr<LoDi>, public winston::Looper
{
public:

	class API
	{
	public:
		enum class DeviceType : unsigned int
		{
			Rektor = 0x03,
			ShiftCommander = 0x09,
			S88Commander = 0x0A
		};

		enum class PacketType : unsigned int
		{
			REQ = 0x20, //: REQ - Anfrage, wird entweder mit ACK, NACK, oder mit BUSY beantwortet
			ACK = 0x21, // - Das REQ - Paket wurde erfolgreich bearbeitet
			EVT = 0x22, // - Automatisch versendetes Paket �ber �nderungen an den Sensoren
			BUSY = 0x23, // - Die Ausf�hrung des REQ - Pakets wurde noch nicht abgeschlossen
			NACK = 0x3F // - Das REQ - Paket ist fehlerhaft.Die Anfrage kann nicht bearbeitet werden.
		};

		enum class Command : uint8_t
		{
			GetVersion = 0x0F,
			DeviceConfigGet = 0x35,
			S88BusModulesGet = 0xA0,
			S88ModulNameGet = 0xA3,
			S88ChannelNamesGet = 0xA5,
			S88MelderGet = 0x30,
			S88LokAddrGet = 0xA7,
			S88CurrentLevelsGet = 0xAA,
			S88EventsActivate = 0x01
		};

		enum class Event : uint8_t
		{
			S88MelderEvent = 0x31,
			S88LokAddrEvent = 0x3C,
			S88RawEvent = 0x3A,
			S88RailcomResponseEvent = 0x3B
		};
	};

	using Payload = std::vector<uint8_t>;

	struct Packet
	{
		Packet(const API::PacketType type, const Payload payload);
		~Packet() = default;
		const API::PacketType type;
		const Payload payload;
	};

	struct PacketCommand
	{
		PacketCommand(const API::Command command, const Payload payload);
		~PacketCommand() = default;
		const API::Command command;
		const Payload payload;
	};

	struct PacketEvent
	{
		PacketEvent(const API::Event event, const Payload payload);
		~PacketEvent() = default;
		const API::Event event;
		const Payload payload;
	};

	LoDi(winston::hal::Socket::Shared socket);
	const winston::Result discover();

	using winston::Shared_Ptr<LoDi>::Shared;
	using winston::Shared_Ptr<LoDi>::make;

private:

	class PacketParser : public winston::Looper
	{
	public:

		using PacketCallback = std::function<const winston::Result(const Payload payload)>;

		struct PacketAndCallback
		{
			Payload data;
			API::Command command{ 0 };
			PacketCallback callback;
			winston::TimePoint sentAt{ winston::hal::now()};
			unsigned int sentCount{ 0 };
		};
		using ExpectedResponses = std::map<uint8_t, PacketAndCallback>;
		ExpectedResponses expectedResponse;

		PacketParser(winston::hal::Socket::Shared socket);
		virtual ~PacketParser() = default;

		const winston::Result loop();

		const winston::Result send(const API::Command command, const Payload payload, PacketCallback callback);
		void setDetectorDevice(winston::DetectorDevice::Shared detectorDevice);
		const bool connected() const;
	protected:
	private:
		const winston::Result sendAgain(PacketAndCallback& packetAndCallback);
		const winston::Result processBuffer();
		const winston::Result processPacket(const Payload payload);
		const winston::Result processEvent(const API::Event event, const Payload payload);
		const uint8_t nextPacketNumber();

		uint8_t packetNumber;
		winston::hal::Socket::Shared socket;

		std::deque<uint8_t> buffer;

		winston::DetectorDevice::Shared detectorDevice;
		bool _connected;
	};

	public:
	const winston::Result loop();

	class S88Commander : public winston::Shared_Ptr<S88Commander>, public winston::DetectorDevice
	{
	public:
		enum class State
		{
			Initializing = 0,
			Ready = 1,
			Unknown = 0xF0
		};

		S88Commander(PacketParser &packetParser, const std::string name);
		virtual ~S88Commander() = default;

		const winston::Result init(PortSegmentMap ports, Callbacks callbacks);
		const State state();
		virtual const bool isReady();
		virtual const bool connected() const;

		const winston::Result getVersion();
		const winston::Result deviceConfigGet();
		const winston::Result s88BusModulesGet();
		const winston::Result s88ModulNameGet();
		const winston::Result s88ChannelNamesGet();
		const winston::Result s88MelderGet();
		const winston::Result s88LokAddrGet(const uint8_t module);
		const winston::Result s88CurrentLevelsGet();
		const winston::Result s88EventsActivate(const bool activate);

		using winston::Shared_Ptr<S88Commander>::Shared;
		using winston::Shared_Ptr<S88Commander>::make;
	private: 
		State _state;

		enum class Initialized : unsigned int
		{
			Uninitialized = 0b0000,
			Version = 0b0001,
			DeviceConfig = 0b0010,
			EventsActive = 0b0100,
			Finished = 0b0111
		};
		unsigned int initializedComponents;
		PacketParser& packetParser;
	};

	S88Commander::Shared createS88Commander();
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
	static winston::hal::DebugSocket::Shared createLoDiDebugSocket();
	static winston::hal::DebugSocket::Packet railcomEvent(const winston::Id segment, const winston::Address loco, const winston::Detector::Change change);
#endif
private:
	PacketParser packetParser;
};