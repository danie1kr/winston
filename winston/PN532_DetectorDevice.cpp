#include "PN532_DetectorDevice.h"

PN532_DetectorDevice::PN532_DetectorDevice(winston::NFCDetector& detector, winston::hal::SerialDevice& device)
	: winston::DetectorDevice<winston::NFCAddress>(detector), winston::Shared_Ptr<PN532_DetectorDevice>(), lastCommand(), lastSend(0), expectedHandler(), firmwareVersion(0), device(device)
{

}

const winston::Result PN532_DetectorDevice::init(unsigned int timeout)
{
    this->wakeup();

    winston::hal::delay(timeout);
    while (this->device.available())
        this->update();

    auto result = this->configureSAM();
    if (result != winston::Result::OK) 
        return result;

    winston::hal::delay(timeout);
    while (this->device.available())
        this->update();

    result = this->getFirmwareVersion();
    if (result != winston::Result::OK)
        return result;

    winston::hal::delay(timeout);
    while (this->device.available())
        this->update();

    result = this->startReadPassiveTargetID(PN532_MIFARE_ISO14443A);
    if (result != winston::Result::OK)
        return result;

    return winston::Result::OK;
}

void PN532_DetectorDevice::dumpAll()
{
    if (this->device.available())
        printf("cleaning COM buffer...");
    while (this->device.available()) {
        auto ret = this->device.read();
        //printf("%02hhx ", (unsigned char)ret);
    }
    printf("\n");
}

void PN532_DetectorDevice::wakeup()
{
    dumpAll();

    //Packet coffee({0x55, 0x55, 0x0, 0x0, 0x0});
  //std::vector<unsigned char> coffee({ 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x14, 0x00});
//works:
    std::vector<unsigned char> coffee({ 0x55, 0x55, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00 });
    //std::vector<unsigned char> coffee({ 0x55, 0x55, 0x00, 0x00, 0x00 });// , 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x03, 0xFD, 0xD4, 0x14, 0x01, 0x17, 0x00
//});

    this->device.send(coffee);
    winston::hal::delay(200);
    dumpAll();
}

const winston::Result PN532_DetectorDevice::configureSAM()
{
    Packet command(4, 0);
    command[0] = PN532_COMMAND_SAMCONFIGURATION;
    command[1] = 0x01; // normal mode;
    command[2] = 0x14; // timeout 50ms * 20 = 1 second - not used in mode 0x01
    command[3] = 0x00; // use IRQ pin!
    auto retVal = this->writeCommand(command);
    this->expectedHandler.push(std::make_pair(PN532_DetectorDevice::sam, 6));

    return retVal;
}

const winston::Result PN532_DetectorDevice::update()
{
    if (this->expectedHandler.size())
    {
        if (this->device.available() >= this->expectedHandler.front().second)
        {
            auto retVal = this->expectedHandler.front().first(*this);
            this->expectedHandler.pop();
            return retVal;
        }
    }
    else
    {
        printf("there is still something in the line we do not expect: ");
        this->dumpAll();
        return winston::Result::OK;
    }
    return winston::Result::ReceiveFailed;
}

const winston::Result PN532_DetectorDevice::getFirmwareVersion()
{
    Packet command(1, 0);
    command[0] = PN532_COMMAND_GETFIRMWAREVERSION;
    auto retVal = this->writeCommand(command);
    if(retVal == winston::Result::OK)
        this->expectedHandler.push(std::make_pair(PN532_DetectorDevice::version, 4));

    return retVal;
    /*
    // read data packet
    int16_t status = HAL(readResponse)(pn532_packetbuffer, sizeof(pn532_packetbuffer));
    if (0 > status) {
        return 0;
    }

    response = pn532_packetbuffer[0];
    response <<= 8;
    response |= pn532_packetbuffer[1];
    response <<= 8;
    response |= pn532_packetbuffer[2];
    response <<= 8;
    response |= pn532_packetbuffer[3];

    return response;*/
}

const winston::Result PN532_DetectorDevice::startReadPassiveTargetID(const unsigned char cardbaudrate)
{
    /*
    pn532_packetbuffer[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    pn532_packetbuffer[1] = 1;
    pn532_packetbuffer[2] = cardbaudrate;

    return this->writeCommand(this->pn532_packetbuffer.data(), 3);*/
    Packet command(3, 0);
    command[0] = PN532_COMMAND_INLISTPASSIVETARGET;
    command[1] = 1;
    command[2] = cardbaudrate;
    auto retVal = this->writeCommand(command);
    if (retVal == winston::Result::OK)
        this->expectedHandler.push(std::make_pair(PN532_DetectorDevice::card, 4));

    return retVal;
}

const winston::Result PN532_DetectorDevice::writeCommand(const std::vector<unsigned char> header)
{

    /** dump serial buffer *
    if (device.available()) {
        DMSG("Dump serial buffer: ");
    }
    while (device.available()) {
        uint8_t ret = device.read();
        DMSG_HEX(ret);
    }*/

    Packet packet;
    packet.reserve(/*8 + */3 + 2 + 1 + header.size() + 2);
    /*
    packet.push_back(0x55);
    packet.push_back(0x55);
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);
    packet.push_back(0x00);*/
    packet.push_back(PN532_PREAMBLE);
    packet.push_back(PN532_STARTCODE1);
    packet.push_back(PN532_STARTCODE2);

    uint8_t length = (uint8_t)header.size() + 1;   // length of data field: TFI + DATA
    packet.push_back(length);
    packet.push_back(~length + 1);         // checksum of length

    packet.push_back(PN532_HOSTTOPN532);
    uint8_t sum = PN532_HOSTTOPN532;    // sum of TFI + DATA

    for (uint8_t i = 0; i < header.size(); i++) {
        packet.push_back(header[i]);
        sum += header[i];
    }

    /*device.write(body, blen);
    for (uint8_t i = 0; i < blen; i++) {
        sum += body[i];
    }*/

    uint8_t checksum = ~sum + 1;            // checksum of TFI + DATA
    packet.push_back(checksum);
    packet.push_back(PN532_POSTAMBLE);

    this->expectedHandler.push(std::make_pair(PN532_DetectorDevice::ack, 6));

    return this->device.send(packet);

    //return readAckFrame();
}

const winston::Result PN532_DetectorDevice::read(const char replyCommand, Packet& content)
{
    std::vector<unsigned char> data;

    /** Frame Preamble and Start Code */
    if (!this->device.read(data, 3))
        return winston::Result::ValidationFailed;
    if (0 != data[0] || 0 != data[1] || 0xFF != data[2]) {
        return winston::Result::ValidationFailed;
    }

    /** receive length and check */
    if (!this->device.read(data, 2))
        return winston::Result::ValidationFailed;
    unsigned char length[2] = { data[0], data[1] };
    if (0 != (char)(length[0] + length[1])) {
        return winston::Result::ValidationFailed;
    }
    length[0] -= 2;

    /** receive command byte */
    if (!this->device.read(data, 2))
        return winston::Result::ValidationFailed;
    if (PN532_PN532TOHOST != data[0] || replyCommand != data[1]) {
        return winston::Result::ValidationFailed;
    }

    unsigned char sum = PN532_PN532TOHOST + replyCommand;
    if (length[0] > 0)
    {
        /** receive data bytes */
        if (!this->device.read(content, length[0]))
            return winston::Result::ValidationFailed;
        for (char i = 0; i < length[0]; i++) {
            sum += content[i];
        }
    }

    /** checksum and postamble */
    if (!this->device.read(data, 2))
        return winston::Result::ValidationFailed;
    if (0 != (uint8_t)(sum + data[0]) || 0 != data[1]) {
        return winston::Result::ValidationFailed;
    }

    return winston::Result::OK;
}

const winston::Result PN532_DetectorDevice::ack(PN532_DetectorDevice& instance)
{
    printf("ack ... ");
    const uint8_t PN532_ACK[] = { 0, 0, 0xFF, 0, 0xFF, 0 };
    for (size_t i = 0; i < sizeof(PN532_ACK); ++i)
        if (instance.device.read() != PN532_ACK[i])
            return winston::Result::ValidationFailed;

    printf("success\n");
    return winston::Result::OK;
}

const winston::Result PN532_DetectorDevice::sam(PN532_DetectorDevice& instance)
{
    printf("sam ... ");
    std::vector<unsigned char> nothing;

    auto result = instance.read(PN532_RESPONSE_SAMCONFIGURATION, nothing);
    if(result == winston::Result::OK)
        printf("success\n");
    return result;
}

const winston::Result PN532_DetectorDevice::version(PN532_DetectorDevice& instance)
{
    printf("version ... ");
    std::vector<unsigned char> fw(4, 0);

    auto result = instance.read(PN532_RESPONSE_GETFIRMWAREVERSION, fw);
    if (result == winston::Result::OK)
    {
        instance.firmwareVersion = (fw[0] << 24) | (fw[1] << 16) | (fw[2] << 8) | fw[3];
        printf("success\n");
    }
    return result;
}

const winston::Result PN532_DetectorDevice::card(PN532_DetectorDevice& instance)
{
    printf("card ... ");
    std::vector<unsigned char> cardData;
    auto result = instance.read(PN532_RESPONSE_INLISTPASSIVETARGET, cardData);
    if(result != winston::Result::OK)
        return result;

    if (cardData[0] != 1)
        return winston::Result::ValidationFailed;

    uint16_t sens_res = cardData[2];
    sens_res <<= 8;
    sens_res |= cardData[3];

    //DMSG("ATQA: 0x");  DMSG_HEX(sens_res);
    //DMSG("SAK: 0x");  DMSG_HEX(pn532_packetbuffer[4]);
    //DMSG("\n");

    /* Card appears to be Mifare Classic */
    size_t cardIdLength = cardData[5];
    unsigned long long id = 0;
    for (size_t i = 0; i < cardIdLength; i++) {
        id <<= 8;
        id |= cardData[6 + i];
    }

    instance.detector.detect(id);
    printf("success\n");

    result = instance.startReadPassiveTargetID(PN532_MIFARE_ISO14443A);
    return result;
}