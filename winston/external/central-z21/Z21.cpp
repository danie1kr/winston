/**
 ** This is Public Domain Software.
 **
 ** The author disclaims copyright to this source code.
 ** In place of a legal notice, here is a blessing:
 **
 **    May you do good and not evil.
 **    May you find forgiveness for yourself and forgive others.
 **    May you share freely, never taking more than you give.
 **/

#include "Z21.h"
#include "Z21Const.h"

#include "../../../libwinston/Winston.h"
#include "../../../libwinston/HAL.h"
#include "../../../libwinston/Log.h"
#include "../../../libwinston/Track.h"

#include <memory>

#define NOT_IMPLEMENTED(func) winston::logger.warn(winston::build("Z21: ", func, " not implemented"));

Z21::Z21(winston::hal::Socket::Shared& socket, winston::DigitalCentralStation::TurnoutAddressTranslator::Shared& addressTranslator, LocoAddressTranslator& locoAddressTranslator, winston::SignalTower::Shared& signalTower, winston::DigitalCentralStation::Callbacks callbacks)
    : winston::DigitalCentralStation(addressTranslator, locoAddressTranslator, signalTower, callbacks), lastMsgSent{}, socket(socket)
{
    this->onBroadcastFlags = [](uint32_t flags) { NOT_IMPLEMENTED("onBroadcastFlags"); };

    this->onBCStopped = []() { winston::logger.info("Z21: Notstop"); };
    this->onStatusChanged = [](uint8_t status) { 
        winston::logger.info("Z21: Status");
        winston::logger.info("     emergency stop: ", (status & Z21_Status::EMERGENCY_STOP) ? "true" : "false");
        winston::logger.info("     track voltage: ", (status & Z21_Status::TRACK_VOLTAGE_OFF) ? "off" : "enabled");
        winston::logger.warn("     short circuit: ", (status & Z21_Status::SHORT_CIRCUIT) ? "detected" : "false");
        winston::logger.info("     programming mode: ", (status & Z21_Status::PROGRAMMING_MODE_ACTIVE) ? "active" : "off");
    };
    this->onSystemStateDataChanged = [this](uint16_t mainCurrent,              // mA
        uint16_t progCurrent,              // mA
        uint16_t mainCurrentFiltered,      // mA
        uint16_t temperature,              // °C
        uint16_t voltageSupply,            // mV
        uint16_t voltageVCC,               // mV
        uint8_t  status,                   // See: const in class Z21_Status
        uint8_t  statisEX) { 
            this->onStatusChanged(status);
            winston::logger.info("     main track current: ", mainCurrent, "mA");
            winston::logger.info("     programming track current: ", progCurrent, "mA");
            winston::logger.info("     main track current filtered: ", mainCurrentFiltered, "mA");
            winston::logger.info("     temperature: ", temperature, "°C");
            winston::logger.info("     voltage supply: ", voltageSupply, "mV");
            winston::logger.info("     voltage track: ", voltageVCC, "mV");
            winston::logger.warn("     high temperature: ", (status & Z21_Status_EX::HIGH_TEMPERATURE) ? "true" : "false");
            winston::logger.warn("     power lost: ", (status & Z21_Status_EX::POWER_LOST) ? "true" : "false");
            winston::logger.warn("     short circuit internal: ", (status & Z21_Status_EX::SHORT_CIRCUIT_EXTERNAL) ? "true" : "false");
            winston::logger.warn("     short circuit external: ", (status & Z21_Status_EX::SHORT_CIRCUIT_INTERNAL) ? "true" : "false");
    };
    this->onUnknownCommand = []() { winston::logger.warn("Z21: onUnknownCommand"); };
    this->onLocoMode = [](uint16_t address, uint8_t decoderMode) { NOT_IMPLEMENTED("onLocoMode"); };
    this->onAccessoryMode = [](uint16_t address, uint8_t decoderMode) { NOT_IMPLEMENTED("onAccessoryMode"); };

    // Programming
    this->onCVResult = [](uint16_t cvAddress, uint8_t value) { NOT_IMPLEMENTED("onCVResult"); };

    this->onCVAccessFailed = []() { NOT_IMPLEMENTED("onCVAccessFailed"); };
    this->onCVAccessFailedShortCircuit = []() { NOT_IMPLEMENTED("onCVAccessFailedShortCircuit"); };


    // Other Networks
    this->onRailComDataChanged = [](uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onRailComDataChanged"); };
    this->onRBusDataChanged = [](uint8_t groupId, uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onRBusDataChanged"); };
    this->onLoconetRX = [](uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onLoconetRX"); };
    this->onLoconetTX = [](uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onLoconetTX"); };
    this->onLoconetFromLAN = [](uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onLoconetFromLAN"); };
    this->onLoconetDispatchAddress = [](uint16_t address, uint8_t result) { NOT_IMPLEMENTED("onLoconetDispatchAddress"); };
    this->onLoconetDetector = [](uint8_t request, uint16_t address, uint8_t* data, uint16_t length) { NOT_IMPLEMENTED("onLoconetDetector"); };

}

const winston::Result Z21::connect()
{
    CaR(this->logoff());
/*#ifdef WINSTON_REALWORLD
    // give the z21 time to boot
    winston::hal::delay(8000);
#else*/
    winston::hal::delay(100);
//#endif
    CaR(this->getStatus());
    /*
    CaR(this->getSerialNumber());
    CaR(this->getHardwareInfo());
    CaR(this->getFirmwareVersion());
    CaR(this->getVersion());
    CaR(this->setBroadcastFlags(this->broadcastFlags));// | Z21_Broadcast::ALL_LOCO_INFO));
    */
    return winston::Result::OK;
}

const winston::Result Z21::connectedInternal()
{
    CaR(this->getSerialNumber());
    CaR(this->getHardwareInfo());
    CaR(this->getFirmwareVersion());
    CaR(this->getVersion());
    CaR(this->setBroadcastFlags(this->broadcastFlags));// | Z21_Broadcast::ALL_LOCO_INFO));
    return winston::Result::OK;
}

const winston::Result Z21::send(Z21Packet& packet)
{
    this->lastMsgSent = winston::hal::now();
    winston::Result result = this->socket->send(packet.data);
    return result;
}

void Z21::keepAlive()
{
    if (winston::hal::now() - this->lastMsgSent > this->keepAliveTimeout)
    {        
        CheckAndReport(this->getStatus());
        CheckAndReport(this->setBroadcastFlags(this->broadcastFlags));
    }
}


bool Z21::isEmergencyStop() const
{
    return this->emergencyStop;
}

const winston::Result Z21::requestEmergencyStop(const bool emergencyStop)
{
    if(emergencyStop)
        return this->send(this->setTrackPowerOff());
    else
        return this->send(this->setTrackPowerOn());
}

const winston::Result Z21::loop()
{
//    if (this->socketListen->isConnected())
    {
        std::vector<unsigned char> data;
        auto result = this->socket->recv(data);
        if (data.size() > 0)
            this->processPacket(data.data());

        this->keepAlive();

        return result;
    }
    //else
      //  return winston::Result::OK;
}

const winston::Result Z21::getSerialNumber() {
    Z21Packet packet;
    packet.setHeader(4, Z21_LAN::GET_SERIAL_NUMBER);
    return this->send(packet);
}

const winston::Result Z21::getHardwareInfo() {
    Z21Packet packet;
    packet.setHeader(4, Z21_LAN::GET_HWINFO);
    return this->send(packet);
}

const winston::Result Z21::getVersion() {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::GET_VERSION, Z21TX_DB0::GET_VERSION);
    return this->send(packet);
}

const winston::Result Z21::getFirmwareVersion() {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::GET_FIRMWARE_VERSION, Z21TX_DB0::GET_FIRMWARE_VERSION);
    return this->send(packet);
}

const winston::Result Z21::getStatus() {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::GET_STATUS, Z21TX_DB0::GET_STATUS);
    return this->send(packet);
}

Z21Packet& Z21::getSystemState() {
    packet.setHeader(4, Z21_LAN::SYSTEMSTATE_GETDATA);
    return packet;
}

Z21Packet& Z21::railcomGetData() {
    packet.setHeader(4, Z21_LAN::RAILCOM_GETDATA);
    return packet;
}

Z21Packet& Z21::setTrackPowerOff() {
    packet.setXPacket(Z21TX_LAN_X::SET_TRACK_POWER_OFF, Z21TX_DB0::SET_TRACK_POWER_OFF);
    return packet;
}

Z21Packet& Z21::setTrackPowerOn() {
    packet.setXPacket(Z21TX_LAN_X::SET_TRACK_POWER_ON, Z21TX_DB0::SET_TRACK_POWER_ON);
    return packet;
}

const winston::Result Z21::getBroadcastFlags() {
    Z21Packet packet;
    packet.setHeader(4, Z21_LAN::GET_BROADCASTFLAGS);
    return this->send(packet);
}

const winston::Result Z21::setBroadcastFlags(uint32_t flags) {
    Z21Packet packet;
    packet.setHeader(8, Z21_LAN::SET_BROADCASTFLAGS);
    packet.setLEuint32(4, flags);
    return this->send(packet);
}

const winston::Result Z21::logoff() {
    Z21Packet packet;
    this->connectedToDCS = false;
    packet.setHeader(4, Z21_LAN::LOGOFF);
    return this->send(packet);
}

const winston::Result Z21::setStop() {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::SET_STOP);
    return this->send(packet);
}

const winston::Result Z21::getLocoInfo (uint16_t address) {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::GET_LOCO_INFO,
                      Z21TX_DB0::GET_LOCO_INFO,
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address));
    return this->send(packet);
}

const winston::Result Z21::setLocoDrive(uint16_t address,
                                       bool  forward,
                                       uint8_t  speedRange,
                                       uint8_t  speed) {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::SET_LOCO_DRIVE,
                      Z21TX_DB0::SET_LOCO_DRIVE | (speedRange & 0xF),
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address),
                      ((forward ? 0x80 : 0x00) | (speed & 0x7F)));
    return this->send(packet);
}

const winston::Result Z21::setLocoFunction(uint16_t address,
                                          uint8_t  function,
                                          uint8_t  action) {
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::SET_LOCO_FUNCTION,
                      Z21TX_DB0::SET_LOCO_FUNCTION,
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address),
                      ((action & 0x03) << 6) | (function & 0x1F));
    return this->send(packet);
}

const winston::Result Z21::getAccessoryInfo(uint16_t address) {
    packet.setXPacket(Z21TX_LAN_X::GET_TURNOUT_INFO,
                      Z21Packet::MSBAccessoryAddress(address),
                      Z21Packet::LSBAccessoryAddress(address));
    return this->send(packet);
}

const winston::Result Z21::setAccessory(uint16_t    address,
                                       uint8_t     pos,
                                       bool     activate,
                                       bool     queue) {
    
    Z21Packet packet;
    packet.setXPacket(Z21TX_LAN_X::SET_TURNOUT,
                      Z21Packet::MSBAccessoryAddress(address),
                      Z21Packet::LSBAccessoryAddress(address),
                      0x80 | (queue ? 0x20: 0) | (activate ? 0x08: 0) | (pos == Z21_Accessory_Pos::P1 ? 0x01 : 0)
                      );
    return this->send(packet);
}

Z21Packet& Z21::getLocoMode(uint16_t address) {
    packet.setHeader(6, Z21_LAN::GET_LOCOMODE);
    packet.setBEuint16(4, address);
    return packet;
}

Z21Packet& Z21::setLocoMode(uint16_t address, uint8_t mode) {
    packet.setHeader(7, Z21_LAN::SET_LOCOMODE);
    packet.setBEuint16(4, address);
    packet.setByte(6, mode);
    return packet;
}

Z21Packet& Z21::getAccessoryMode(uint16_t address) {
    packet.setHeader(6, Z21_LAN::GET_TURNOUTMODE);
    packet.setBEuint16(4, address);
    return packet;
}

Z21Packet& Z21::setAccessoryMode(uint16_t address, uint8_t mode) {
    packet.setHeader(7, Z21_LAN::SET_TURNOUTMODE);
    packet.setBEuint16(4, address);
    packet.setByte(6, mode);
    return packet;
}

Z21Packet& Z21::dccReadRegister(uint8_t reg) {
    packet.setXPacket(Z21TX_LAN_X::DCC_READ_REGISTER,
                      Z21TX_DB0::DCC_READ_REGISTER,
                      reg);
    return packet;
}

Z21Packet& Z21::dccWriteRegister(uint8_t reg, uint8_t value) {
    packet.setXPacket(Z21TX_LAN_X::DCC_WRITE_REGISTER,
                      Z21TX_DB0::DCC_WRITE_REGISTER,
                      reg,
                      value);
    return packet;
}

Z21Packet& Z21::dccReadCV(uint16_t cv) {
    packet.setXPacket(Z21TX_LAN_X::CV_READ,
                      Z21TX_DB0::CV_READ,
                      Z21Packet::MSBCVAddress(cv),
                      Z21Packet::LSBCVAddress(cv));
    return packet;
}

Z21Packet& Z21::dccWriteCV(uint16_t cv, uint8_t value) {
    packet.setXPacket(Z21TX_LAN_X::CV_WRITE,
                      Z21TX_DB0::CV_WRITE,
                      Z21Packet::MSBCVAddress(cv),
                      Z21Packet::LSBCVAddress(cv),
                      value);
    return packet;
}

Z21Packet& Z21::mmWriteByte(uint8_t reg, uint8_t value) {
    packet.setXPacket(Z21TX_LAN_X::MM_WRITE_BYTE,
                      Z21TX_DB0::MM_WRITE_BYTE,
                      0,
                      reg,
                      value);
    return packet;
}

Z21Packet& Z21::dccReadOnMainLocoCVByte (uint16_t address, uint16_t cv) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_READ_BYTE,
                      Z21TX_DB0::CV_POM_READ_BYTE,
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_READ_BYTE,
                      Z21Packet::LSBCVAddress(cv),
                      0);
    return packet;
}
Z21Packet& Z21::dccWriteOnMainLocoCVByte(uint16_t address, uint16_t cv, uint8_t value) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_WRITE_BYTE,
                      Z21TX_DB0::CV_POM_WRITE_BYTE,
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_WRITE_BYTE,
                      Z21Packet::LSBCVAddress(cv),
                      value);
    return packet;
}
Z21Packet& Z21::dccWriteOnMainLocoCVBit (uint16_t address, uint16_t cv, uint8_t bit, bool value) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_WRITE_BIT,
                      Z21TX_DB0::CV_POM_WRITE_BIT,
                      Z21Packet::MSBLocoAddress(address),
                      Z21Packet::LSBLocoAddress(address),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_WRITE_BIT,
                      Z21Packet::LSBCVAddress(cv),
                      (value ? 0x08 : 0) | (bit & 0x7));
    return packet;
}

Z21Packet& Z21::dccReadOnMainAccessoryCVByte(uint16_t address, uint8_t pos, uint16_t cv) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_ACCESSORY_READ_BYTE,
                      Z21TX_DB0::CV_POM_ACCESSORY_READ_BYTE,
                      Z21Packet::MSBAccessoryPosition(address),
                      Z21Packet::LSBAccessoryPosition(address, pos),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_ACCESSORY_READ_BYTE,
                      Z21Packet::LSBCVAddress(cv),
                      0);
    return packet;
}

Z21Packet& Z21::dccWriteOnMainAccessoryCVByte(uint16_t address, uint8_t pos, uint16_t cv, uint8_t value) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_ACCESSORY_WRITE_BYTE,
                      Z21TX_DB0::CV_POM_ACCESSORY_WRITE_BYTE,
                      Z21Packet::MSBAccessoryPosition(address),
                      Z21Packet::LSBAccessoryPosition(address, pos),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_ACCESSORY_WRITE_BYTE,
                      Z21Packet::LSBCVAddress(cv),
                      value);
    return packet;
}

Z21Packet& Z21::dccWriteOnMainAccessoryCVBit (uint16_t address, uint8_t pos, uint16_t cv, uint8_t bit, bool value) {
    packet.setXPacket(Z21TX_LAN_X::CV_POM_ACCESSORY_WRITE_BIT,
                      Z21TX_DB0::CV_POM_ACCESSORY_WRITE_BIT,
                      Z21Packet::MSBAccessoryPosition(address),
                      Z21Packet::LSBAccessoryPosition(address, pos),
                      Z21Packet::MSBCVAddress(cv) | Z21TX_Option::CV_POM_ACCESSORY_WRITE_BIT,
                      Z21Packet::LSBCVAddress(cv),
                      (value ? 0x08 : 0) | (bit & 0x7));
    return packet;
}

Z21Packet& Z21::rbusGetData(uint8_t group) {
    packet.setHeader(5, Z21_LAN::RMBUS_GETDATA);
    packet.setByte(4, group);
    return packet;
}

Z21Packet& Z21::rbusProgramModule(uint8_t addr) {
    packet.setHeader(5, Z21_LAN::RMBUS_PROGRAMMODULE);
    packet.setByte(4, addr);
    return packet;
}

Z21Packet& Z21::loconetDispatchAddress(uint16_t address) {
    packet.setHeader(6, Z21_LAN::LOCONET_DISPATCH_ADDR);
    packet.setLEuint16(4, address);
    return packet;
}

Z21Packet& Z21::loconetDetector(uint8_t type, uint16_t address) {
    packet.setHeader(7, Z21_LAN::LOCONET_DETECTOR);
    packet.setByte(4, type);
    packet.setLEuint16(5, address);
    return packet;
}

Z21Packet& Z21::loconetSendMessage(uint16_t messageLength) {
    packet.setHeader(4 + messageLength, Z21_LAN::LOCONET_FROM_LAN);
    return packet;
}

void Z21::processPacket(uint8_t* data) {
    auto header = Z21Packet::getHeader(data);
    switch (header) {
        case Z21_LAN::X:
            processXPacket(data);
            break;
        case Z21_LAN::GET_SERIAL_NUMBER:
            this->callbacks.systemInfoCallback(header, "Serial Number", winston::build(Z21Packet::getLEuint32(data, 4)));
            //if (onSerialNumber != NULL)
            //    onSerialNumber();
            break;
        case Z21_LAN::GET_HWINFO:
            this->callbacks.systemInfoCallback(header, "Hardware Info", winston::build(Z21Packet::getLEuint32(data, 4), Z21Packet::getLEuint32(data, 8)));
            //if (onHardwareInfo != NULL)
            //    onHardwareInfo(Z21Packet::getLEuint32(data, 4), Z21Packet::getLEuint32(data, 8));
            break;
        case Z21_LAN::GET_BROADCASTFLAGS:
            if (onBroadcastFlags != NULL)
                onBroadcastFlags(Z21Packet::getLEuint32(data, 4));
            break;
        case Z21_LAN::GET_LOCOMODE:
            if (onLocoMode != NULL)
                onLocoMode(Z21Packet::getBEuint16(data, 4), Z21Packet::getByte(data, 6));
            break;
        case Z21_LAN::GET_TURNOUTMODE:
            if (onAccessoryMode != NULL)
                onAccessoryMode(Z21Packet::getBEuint16(data, 4), Z21Packet::getByte(data, 6));
            break;
        case Z21_LAN::RMBUS_DATACHANGED:
            if (onRBusDataChanged != NULL)
                onRBusDataChanged(Z21Packet::getByte(data, 4), data + 5, Z21Packet::getLength(data) - 5);
            break;
        case Z21_LAN::RAILCOM_DATACHANGED:
            if (onRailComDataChanged != NULL)
                onRailComDataChanged(data + 4, Z21Packet::getLength(data) - 4);
            break;
        case Z21_LAN::SYSTEMSTATE_DATACHANGED:
            if (onSystemStateDataChanged != NULL)
                onSystemStateDataChanged(Z21Packet::getLEuint16(data, 4),
                                         Z21Packet::getLEuint16(data, 6),
                                         Z21Packet::getLEuint16(data, 8),
                                         Z21Packet::getLEuint16(data, 10),
                                         Z21Packet::getLEuint16(data, 12),
                                         Z21Packet::getLEuint16(data, 14),
                                         Z21Packet::getByte(data, 16),
                                         Z21Packet::getByte(data, 17));
            break;
        case Z21_LAN::LOCONET_Z21_RX:
            if (onLoconetRX != NULL)
                onLoconetRX(data + 4, Z21Packet::getLength(data) - 4);
            break;
        case Z21_LAN::LOCONET_Z21_TX:
            if (onLoconetTX != NULL)
                onLoconetTX(data + 4, Z21Packet::getLength(data) - 4);
            break;
        case Z21_LAN::LOCONET_FROM_LAN:
            if (onLoconetFromLAN != NULL)
                onLoconetFromLAN(data + 4, Z21Packet::getLength(data) - 4);
            break;
        case Z21_LAN::LOCONET_DISPATCH_ADDR:
            if (onLoconetDispatchAddress != NULL)
                onLoconetDispatchAddress(Z21Packet::getLEuint16(data, 4), Z21Packet::getByte(data, 6));
            break;
        case Z21_LAN::LOCONET_DETECTOR:
            if (onLoconetDetector != NULL)
                onLoconetDetector(Z21Packet::getByte(data, 4), Z21Packet::getLEuint16(data, 5), data + 7, Z21Packet::getLength(data) - 7);
            break;
        default:
            break;
    }
}

void Z21::requestTurnoutInfo(winston::Turnout& turnout)
{
    const unsigned int address = this->turnoutAddressTranslator->address(turnout);
    this->getAccessoryInfo((const unsigned short)address);
}

void Z21::requestDoubleSlipTurnoutInfo(winston::DoubleSlipTurnout& turnout)
{
    const unsigned int address = this->turnoutAddressTranslator->address(turnout);
    this->getAccessoryInfo((const unsigned short)address);
    this->getAccessoryInfo((const unsigned short)(address+1));
}

void Z21::requestLocoInfo(const winston::Locomotive& loco)
{
    const unsigned int address = this->locoAddressTranslator.addressOfLoco(loco);
    this->getLocoInfo(address);
}

void Z21::triggerTurnoutChangeTo(winston::Turnout& turnout, winston::Turnout::Direction direction)
{
    const unsigned int address = this->turnoutAddressTranslator->address(turnout);
    this->setAccessory((const unsigned short)address, (direction == winston::Turnout::Direction::A_C ? Z21_Accessory_Pos::P0 : Z21_Accessory_Pos::P1), true, true);
}

void Z21::triggerDoubleSlipTurnoutChangeTo(winston::DoubleSlipTurnout& turnout, winston::DoubleSlipTurnout::Direction direction)
{
    const unsigned int addressA = this->turnoutAddressTranslator->address(turnout);
    const unsigned int addressB = addressA + 1;
    unsigned char a, b;
    turnout.toAccessoryStates(a, b, direction);
    this->setAccessory((const unsigned short)addressA, (a ? Z21_Accessory_Pos::P0 : Z21_Accessory_Pos::P1), true, true);
    this->setAccessory((const unsigned short)addressB, (b ? Z21_Accessory_Pos::P0 : Z21_Accessory_Pos::P1), true, true);
}


void Z21::triggerLocoDrive(const winston::Address address, const unsigned char speed, const bool forward)
{
    this->setLocoDrive(address, forward, Z21_Speed_Range::STEPS_128, speed);
}

void Z21::triggerLocoFunction(const winston::Address address, const uint32_t functions)
{
    this->setLocoFunction(address, Z21_Function::F0, functions & 0b1 ? Z21_Function_Action::ON : Z21_Function_Action::OFF);
}

void Z21::processXPacket(uint8_t* data) {
    auto header = Z21Packet::getXHeader(data);
    switch (header) {
        case Z21RX_LAN_X::BC:
            processBCPacket(data);
            break;
        case Z21RX_LAN_X::TURNOUT_INFO:
            {
                const uint16_t address = Z21Packet::getBEuint16(data, 5);
                const uint8_t accessoryState = Z21Packet::getByte(data, 7);

                if (this->callbacks.specialAccessoryProcessingCallback(address, accessoryState))
                {
                    auto track = this->turnoutAddressTranslator->turnout(address);
                    if (track->type() == winston::Track::Type::Turnout)
                    {
                        auto turnout = std::static_pointer_cast<winston::Turnout>(track);
                        if (accessoryState == Z21_Accessory_State::P0 || accessoryState == Z21_Accessory_State::P1)
                        {
                            auto direction = accessoryState == Z21_Accessory_State::P1 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C;
                            this->turnoutUpdate(*turnout, direction);
                            winston::logger.log(winston::build("Z21: Turnout ", address, " in state ", winston::Turnout::DirectionToString(direction)));
                        }
                        else
                        {
                            winston::logger.err(winston::build("Z21: Turnout ", address, " in illegal state ", accessoryState));
                        }
                    }
                    else if (track->type() == winston::Track::Type::DoubleSlipTurnout)
                    {
                        auto turnout = std::static_pointer_cast<winston::DoubleSlipTurnout>(track);
                        // returns first address
                        bool firstAddress = this->turnoutAddressTranslator->address(*turnout) == address;
                        if (accessoryState == Z21_Accessory_State::P0 || accessoryState == Z21_Accessory_State::P1)
                        {
                            unsigned char state = accessoryState == Z21_Accessory_State::P0 ? 1 : 0;
                            if (turnout->isKnownAccessoryState())
                            {
                                // we do this twice potentially...
                                turnout->setAccessoryState(state, firstAddress, false, false);
                                auto direction = turnout->fromAccessoryState();
                                this->doubleSlipUpdate(*turnout, direction);
                                winston::logger.log(winston::build("Z21: Turnout ", address, " in state ", winston::DoubleSlipTurnout::DirectionToString(direction)));
                            }
                            else
                            {
                                turnout->setAccessoryState(state, firstAddress, true, true);
                            }
                        }
                        else
                        {
                            winston::logger.err(winston::build("Z21: Turnout ", address, " in illegal state ", accessoryState));
                        }
                    }
                }
                break;
            }
        case Z21RX_LAN_X::STATUS_CHANGED:
            if (onStatusChanged != NULL && Z21Packet::getDB0(data) == Z21RX_DB0::STATUS_CHANGED)
            {
                if (!this->connected())
                    this->onConnected();
                onStatusChanged(Z21Packet::getDB1(data));
            }
            break;
        case Z21RX_LAN_X::GET_VERSION:
            this->callbacks.systemInfoCallback(header, "Version", winston::build(Z21Packet::getDB1(data), Z21Packet::getDB2(data)));
            break;
        case Z21RX_LAN_X::CV_RESULT:
            if (onCVResult != NULL && Z21Packet::getDB0(data) == Z21RX_DB0::CV_RESULT)
                onCVResult(Z21Packet::getBEuint16(data, 6), Z21Packet::getDB3(data));
            break;
        case Z21RX_LAN_X::BC_STOPPED:
            if (onBCStopped != NULL)
                onBCStopped();
            break;
        case Z21RX_LAN_X::LOCO_INFO:
            processLocoInfo(data);
            break;
        case Z21RX_LAN_X::GET_FIRMWARE_VERSION:
            this->callbacks.systemInfoCallback(header, "Firmware Version", winston::build(Z21Packet::getDB1(data), Z21Packet::getDB2(data)));
            break;
        default:
            break;
    }
}

void Z21::processLocoInfo(uint8_t* data) {
    uint16_t addr = Z21Packet::getBEuint16(data, 5) & 0x3FFF;

    uint8_t XDB2 = Z21Packet::getDB2(data);
    uint8_t XDB3 = Z21Packet::getDB3(data);
    uint8_t XDB4 = Z21Packet::getDB4(data);
    uint32_t XDB5 = Z21Packet::getDB5(data);
    uint32_t XDB6 = Z21Packet::getDB6(data);
    uint32_t XDB7 = Z21Packet::getDB7(data);

    bool busy      = Z21Packet::hasBit(XDB2, 0x08);
    bool forward   = Z21Packet::hasBit(XDB3, 0x80);
    // bool consist   = Z21Packet::hasBit(XDB4, 0x40);
    // bool transpond = Z21Packet::hasBit(XDB4, 0x20);

    int speed = (XDB3 & 0x7F);
    if (speed > 1) {
        int speedFormat = (XDB2 & 0x7);
        if (speedFormat == 0) // 14 steps
            speed = (speed << 3);
        if (speedFormat == 2) // 28 steps
            speed = (speed << 2);
        // if (speedFormat == 4) // 128 steps
    }

    uint32_t functions = ((XDB4 & 0x10)>>4);//F0
    functions |= ((XDB4 & 0x0F)<<1);        //F4 - F1
    functions |= (XDB5 << 5);               //F5 - F12
    functions |= (XDB6 << 13);              //F13 - F20
    functions |= (XDB7 << 21);              //F21 - F28

    //onLocoInfo(addr, busy, consist, transpond, forward, speed, functions);
    if (auto loco = this->locoAddressTranslator.locoFromAddress(addr))
        this->callbacks.locomotiveUpdateCallback(*loco, busy, forward, speed, functions);
}

void Z21::processBCPacket(uint8_t* data) {
    auto header = Z21Packet::getDB0(data);
    switch (header) {
        case Z21RX_DB0::BC_TRACK_POWER_OFF:
            this->callbacks.trackPowerStatusCallback(false);
            break;

        case Z21RX_DB0::BC_TRACK_POWER_ON:
            this->callbacks.trackPowerStatusCallback(true);
            break;

        case Z21RX_DB0::BC_PROGRAMMING_MODE:
            this->callbacks.programmingTrackStatusCallback(true);
            break;

        case Z21RX_DB0::BC_TRACK_SHORT_CIRCUIT:
            this->callbacks.shortCircuitDetectedCallback();
            break;

        case Z21RX_DB0::CV_NACK_SC:
            if (onCVAccessFailedShortCircuit != NULL)
                onCVAccessFailedShortCircuit();
            break;

        case Z21RX_DB0::CV_NACK:
            if (onCVAccessFailed != NULL)
                onCVAccessFailed();
            break;

        case Z21RX_DB0::UNKNOWN_COMMAND:
            if (onUnknownCommand != NULL)
                onUnknownCommand();
            break;

        default:
            break;
    }
}
