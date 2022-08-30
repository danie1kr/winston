#pragma once

#include <queue>
#include "../libwinston/Detector.h"

#ifdef WINSTON_NFC_DETECTORS

// see https://github.com/elechouse/PN532/blob/PN532_HSU/PN532/PN532.h
constexpr auto PN532_COMMAND_GETFIRMWAREVERSION = (0x02);
constexpr auto PN532_RESPONSE_GETFIRMWAREVERSION = (0x03);
constexpr auto PN532_COMMAND_SAMCONFIGURATION = (0x14);
constexpr auto PN532_RESPONSE_SAMCONFIGURATION = (0x15);
constexpr auto PN532_COMMAND_INLISTPASSIVETARGET = (0x4A);
constexpr auto PN532_RESPONSE_INLISTPASSIVETARGET = (0x4B);
constexpr auto PN532_MIFARE_ISO14443A = (0x00);

// see https://github.com/elechouse/PN532/blob/PN532_HSU/PN532/PN532Interface.h
constexpr auto PN532_PREAMBLE = (0x00);
constexpr auto PN532_STARTCODE1 = (0x00);
constexpr auto PN532_STARTCODE2 = (0xFF);
constexpr auto PN532_POSTAMBLE = (0x00);
constexpr auto PN532_HOSTTOPN532 = (0xD4);
constexpr auto PN532_PN532TOHOST = (0xD5);

// space below locos:
// middle 20x20mm up to 25x25mm
// drehgestell: 10x30 up to 10x40
class PN532_DetectorDevice : public winston::DetectorDevice<winston::NFCAddress>, public winston::Shared_Ptr<PN532_DetectorDevice>
{
public:
	PN532_DetectorDevice(winston::NFCDetector& detector, winston::hal::SerialDevice& device);

	const winston::Result init(unsigned int timeout = 500);
	const winston::Result update();

	using Shared_Ptr<PN532_DetectorDevice>::Shared;
	using Shared_Ptr<PN532_DetectorDevice>::make;
private:
	const winston::Result configureSAM();
	const winston::Result getFirmwareVersion();
	const winston::Result startReadPassiveTargetID(const unsigned char cardbaudrate);
	const winston::Result writeCommand(const std::vector<unsigned char> header);

	using Packet = std::vector<unsigned char>;
	using Response = std::pair<std::function<const winston::Result(PN532_DetectorDevice& instance)>, size_t>;

	static const winston::Result ack(PN532_DetectorDevice& instance);
	static const winston::Result sam(PN532_DetectorDevice& instance);
	static const winston::Result card(PN532_DetectorDevice& instance);
	static const winston::Result version(PN532_DetectorDevice& instance);

	void wakeup();
	void dumpAll();
	const winston::Result read(const char reply, Packet& content);

	Packet lastCommand;
	unsigned long long lastSend;
	std::queue<Response> expectedHandler;

	unsigned int firmwareVersion;
	winston::hal::SerialDevice& device;
};
#endif

