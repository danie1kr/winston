// winston-simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <functional>

#include "../libwinston/Winston.h"

#include "winston.h"
#include "winston-minnow.h"
#include "railways.h"
#include "winston-hal-x64.h"
#include "external/central-z21/Z21.h"

//#define RAILWAY_CLASS RailwayWithSiding
//#define RAILWAY_CLASS TimeSaverRailway
#define RAILWAY_CLASS Y2020Railway

class MRS : public winston::ModelRailwaySystem<RAILWAY_CLASS::Shared, RAILWAY_CLASS::AddressTranslator::Shared, Z21::Shared>
{
private:

    // send a turnout state via websocket
    void turnoutSendState(unsigned int turnoutSectionId, winston::Turnout::Direction dir)
    {
        SendData sd;
        minnowSendPrepare(this->minnowCD, &sd, "turnoutState");
        JEncoder_beginObject(&sd.encoder);
        JEncoder_setName(&sd.encoder, "id");
        JEncoder_setInt(&sd.encoder, turnoutSectionId);
        JEncoder_setName(&sd.encoder, "state");
        JEncoder_setInt(&sd.encoder, (int)dir);

        JEncoder_endObject(&sd.encoder);
        minnowSendSubmit(&sd);
    }

    void locoSend(winston::Locomotive::Shared& loco)
    {
        /*{
            address
            name
            light
            forward
            speed
        }*/
        SendData sd;
        minnowSendPrepare(this->minnowCD, &sd, "loco");
        JEncoder_beginObject(&sd.encoder);
        JEncoder_setName(&sd.encoder, "address");
        JEncoder_setInt(&sd.encoder, loco->address());
        JEncoder_setName(&sd.encoder, "name");
        JEncoder_setString(&sd.encoder, loco->name().c_str());
        JEncoder_setName(&sd.encoder, "light");
        JEncoder_setBoolean(&sd.encoder, loco->light());
        JEncoder_setName(&sd.encoder, "forward");
        JEncoder_setBoolean(&sd.encoder, loco->forward());
        JEncoder_setName(&sd.encoder, "speed");
        JEncoder_setInt(&sd.encoder, loco->speed());
        JEncoder_endObject(&sd.encoder);
        minnowSendSubmit(&sd);
    }

    void locoSend(winston::Address address)
    {
        if (auto loco = this->get(address))
        {
            locoSend(loco);
        }
    }

    // message from websocket received
    int minnow_manageMessage(RecData* o, ConnData* cd, const char* msg, JErr* error, JVal* value)
    {
        if (std::string("turnoutToggle").compare(msg) == 0)
        {
            unsigned int id = 0;
            JVal_get(value, error, "{d}", "id", &id);
            if (JErr_isError(error) == false)
            {
                auto turnout = std::dynamic_pointer_cast<winston::Turnout>(railway->section(id));
                auto injectDir = winston::Turnout::otherDirection(turnout->direction());
                //ConnData** mCD = &this->minnowCD;
                auto cb = std::make_shared<winston::Callback>([this, id, turnout, injectDir]() {
                    this->turnoutSendState(id, turnout->direction());
                    this->stationDebugInjector->injectTurnoutUpdate(turnout, injectDir);
                    });
                //signalBox->notify(std::make_unique<winston::EventTurnoutStartToggle>(cb, turnout));
                signalBox->order(winston::Command::make([this, id, turnout, injectDir](const unsigned long& created) -> const winston::State
                {
                    signalBox->order(winston::Command::make([this, turnout, injectDir](const unsigned long& created) -> const winston::State
                    {
                        if (winston::hal::now()-created > 2000)
                        {
                            this->stationDebugInjector->injectTurnoutUpdate(turnout, injectDir);
                            return winston::State::Finished;
                        }

                        return winston::State::Running;
                        
                    }));

                    //this->turnoutSendState(id, turnout->direction());
                    return turnout->startToggle();
                }));
                return false;
            }
            return true;
        }
        else if (std::string("turnoutState").compare(msg) == 0)
        {
            unsigned int id = 0;
            JVal_get(value, error, "{d}", "id", &id);
            if (JErr_isError(error) == false)
            {
                auto turnout = std::dynamic_pointer_cast<winston::Turnout>(railway->section(id));
                this->turnoutSendState(id, turnout->direction());
                return false;
            }
            return true;
        }
        else if (std::string("getRailway").compare(msg) == 0)
        {
            SendData sd;
            minnowSendPrepare(this->minnowCD, &sd, "railway");

            JEncoder_beginObject(&sd.encoder);
            JEncoder_setName(&sd.encoder, "sections");
            JEncoder_beginArray(&sd.encoder);
            for (unsigned int i = 0; i < railway->sectionsCount(); ++i)
            {
                auto section = railway->section(i);
                switch (section->type())
                {
                case winston::Section::Type::Bumper:
                {
                    winston::Bumper::Shared bumper = std::dynamic_pointer_cast<winston::Bumper>(section);
                    winston::Section::Shared a;
                    bumper->connections(a);
                    JEncoder_set(&sd.encoder, "{d}", "a", railway->sectionIndex(a));
                    break;
                }
                case winston::Section::Type::Rail:
                {
                    winston::Rail::Shared rail = std::dynamic_pointer_cast<winston::Rail>(section);
                    winston::Section::Shared a, b;
                    rail->connections(a, b);
                    JEncoder_set(&sd.encoder, "{dd}", "a", railway->sectionIndex(a), "b", railway->sectionIndex(b));
                    break;
                }
                case winston::Section::Type::Turnout:
                {
                    winston::Turnout::Shared turnout = std::dynamic_pointer_cast<winston::Turnout>(section);
                    winston::Section::Shared a, b, c;
                    turnout->connections(a, b, c);
                    JEncoder_set(&sd.encoder, "{ddd}", "a", railway->sectionIndex(a), "b", railway->sectionIndex(b), "c", railway->sectionIndex(c));
                    break;
                }
                }
            }
            JEncoder_endArray(&sd.encoder);
            JEncoder_endObject(&sd.encoder);
            minnowSendSubmit(&sd);
        }
        else if (std::string("storeRailwayLayout").compare(msg) == 0)
        {
            unsigned int address = 0;
            auto layout = std::string(JVal_getString(value, error));
            auto length = layout.size();

            winston::hal::storageWrite(address + 0, (length >> 0) & 0xFF);
            winston::hal::storageWrite(address + 1, (length >> 8) & 0xFF);
            winston::hal::storageWrite(address + 2, (length >> 16) & 0xFF);
            winston::hal::storageWrite(address + 3, (length >> 24) & 0xFF);

            address = 4;
            for (auto s : layout)
                winston::hal::storageWrite(address++, s);

            winston::hal::storageCommit();

            SendData sd;
            minnowSendPrepare(this->minnowCD, &sd, "storeRailwayLayoutSuccessful");
            JEncoder_setBoolean(&sd.encoder, true);
            minnowSendSubmit(&sd);
        }
        else if (std::string("getRailwayLayout").compare(msg) == 0)
        {
            size_t address = 0;
            size_t length = (winston::hal::storageRead(address + 0) << 0) |
                (winston::hal::storageRead(address + 1) << 8) |
                (winston::hal::storageRead(address + 2) << 16) |
                (winston::hal::storageRead(address + 3) << 24);
            address = 4;

            const size_t sizePerMessage = size_t(0.7f * WinstonMinnowBufferSize);
            size_t remaining = length;
            size_t offset = 0;

            while (remaining > 0)
            {
                size_t sent = remaining > sizePerMessage ? sizePerMessage : remaining;
                auto layout = std::string(sent, '0');

                for (size_t i = 0; i < sent; ++i)
                    layout[i] = winston::hal::storageRead(address + offset + i);

                SendData sd;
                minnowSendPrepare(this->minnowCD, &sd, "layout");
                JEncoder_beginObject(&sd.encoder);
                JEncoder_setName(&sd.encoder, "offset");
                JEncoder_setInt(&sd.encoder, (unsigned int)offset);
                JEncoder_setName(&sd.encoder, "fullSize");
                JEncoder_setInt(&sd.encoder, (unsigned int)length);
                JEncoder_setName(&sd.encoder, "layout");
                JEncoder_setString(&sd.encoder, layout.c_str());
                JEncoder_endObject(&sd.encoder);
                minnowSendSubmit(&sd);

                offset += sent;
                remaining -= sent;
            }

        }
        else if (std::string("getLocoShed").compare(msg) == 0)
        {
            for (auto& loco : this->locomotiveShed)
                this->locoSend(loco);
        }
        else if (std::string("controlLoco").compare(msg) == 0)
        {
            /*
            {
                address
                light
                forward
                speed
            }
            */
            winston::Address address;
            bool light, forward;
            unsigned int speed;
            JVal_get(value, error, "{dbbd}", "address", &address, "light", &light, "forward", &forward, "speed", &speed);

            if (JErr_isError(error) == false)
                if (auto loco = this->get(address))
                {
                    loco->light(light);
                    loco->drive(forward, (unsigned char)(speed & 0xFF));
                }
        }
        else
        {
            winston::hal::text("Received unknown message: ");
            winston::hal::text(msg);
            return -1;
        }

        return 0;
    }

    // reply a html page
    int minnow_fetchPage(MST* mst, const char* path, FetchPageSend minnow_send)
    {
        std::string p(path);

        const std::string path_index("/");
        const std::string path_railway("/railway");

        const std::string header_html("\r\ncontent-type: text/html; charset=UTF-8\r\n");
        const std::string header_json("\r\ncontent-type: application/json; charset=UTF-8\r\n");

        if (p.compare(path_index) == 0)
        {
            minnow_send(mst, header_html.c_str(), "<html>winston</html>");
        }
        else if (p.compare(path_railway) == 0)
        {
            minnow_send(mst, header_json.c_str(), "{}");
        }
        else
            return 0;

        return 1;
    }

    void initNetwork()
    {
        // z21
        z21Socket = UDPSocketLWIP::make(z21IP, z21Port);
        
        // webSocket
        minnowStart(webSocketListenPtr, webSocketSendPtr, &this->minnowWPH, &this->minnowCD, &this->minnowRD, &this->minnowServer, [this](struct RecData* o, struct ConnData* cd, const char* msg, JErr* e, JVal* v)->int { return this->minnow_manageMessage(o, cd, msg, e, v); }, [this](MST* mst, const char* path, FetchPageSend send)->int {return this->minnow_fetchPage(mst, path, send);  });
        this->webSocketState = winston::hal::UDPSocket::State::Connecting;
    }

    winston::DigitalCentralStation::Callbacks z21Callbacks()
    {
        winston::DigitalCentralStation::Callbacks callbacks;

        // what to do when the digital central station updated a turnout
        callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
        {
            //auto id = this->railway->sectionIndex(turnout);
            //turnoutSendState(id, direction);
            turnout->finalizeChangeTo(direction);
            return winston::State::Finished;
        };

        return callbacks;
    }

    winston::Railway::Callbacks railwayCallbacks()
    {
        winston::Railway::Callbacks callbacks;

        callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
        {
            auto id = this->railway->sectionIndex(turnout);
            turnoutSendState(id, direction);
            return winston::State::Finished;
        };

        return callbacks;
    }

    // setup our model railway system
    void systemSetup() { 
        this->initNetwork();

        // the user defined railway and its address translator
        this->railway = RAILWAY_CLASS::make(railwayCallbacks());
        this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

        // the internal signal box
        winston::Railway::Shared rw = std::dynamic_pointer_cast<winston::Railway>(railway);
        this->signalBox = winston::SignalBox::make(rw, nullMutex);

        // the system specific digital central station
        auto at = std::dynamic_pointer_cast<winston::DigitalCentralStation::AddressTranslator>(addressTranslator);
        auto udp = std::dynamic_pointer_cast<winston::hal::UDPSocket>(this->z21Socket);
        this->digitalCentralStation = Z21::make(udp, at, this->signalBox, z21Callbacks());

        // a debug injector
        auto dcs = std::dynamic_pointer_cast<winston::DigitalCentralStation>(this->digitalCentralStation);
        this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(dcs);
    };

    void systemSetupComplete()
    {
        for (size_t i = 0; i < this->railway->sectionsCount(); ++i)
        {
            auto section = this->railway->section(magic_enum::enum_value<RAILWAY_CLASS::Sections>(i));
            if (section->type() == winston::Section::Type::Turnout)
            {
                auto turnout = std::dynamic_pointer_cast<winston::Turnout>(section);
                this->digitalCentralStation->requestTurnoutInfo(turnout);
            }
        }
    }

    // accept new requests and loop over what the signal box has to do
    void systemLoop() {
        if(this->webSocketState == winston::hal::UDPSocket::State::Connecting && minnowAccept(webSocketListenPtr, 2, webSocketSendPtr, this->minnowServer, this->minnowWPH) == 1)
            this->webSocketState = winston::hal::UDPSocket::State::Connected;
            
        this->signalBox->work();

        if (this->webSocketState == winston::hal::UDPSocket::State::Connected && !minnowLoop(webSocketListenPtr, webSocketSendPtr, this->minnowWPH, this->minnowCD, this->minnowRD, this->minnowServer))
        {
            this->webSocketState = winston::hal::UDPSocket::State::Closing;
            minnowClose(webSocketListenPtr, webSocketSendPtr);
            minnowStart(webSocketListenPtr, webSocketSendPtr, &this->minnowWPH, &this->minnowCD, &this->minnowRD, &this->minnowServer, [this](struct RecData* o, struct ConnData* cd, const char* msg, JErr* e, JVal* v)->int { return this->minnow_manageMessage(o, cd, msg, e, v); }, [this](MST* mst, const char* path, FetchPageSend send)->int {return this->minnow_fetchPage(mst, path, send);  });
            this->webSocketState = winston::hal::UDPSocket::State::Connecting;
        }
    }

    void populateLocomotiveShed()
    {
        this->addLocomotive(1, "BR 114");
        this->addLocomotive(4, "BR 106");
        this->addLocomotive(5, "BR 64");
    }

    // minnow related
    WssProtocolHandshake* minnowWPH = nullptr;
    ConnData* minnowCD = nullptr;
    RecData* minnowRD = nullptr;
    MS* minnowServer = nullptr;

    UDPSocketLWIP::Shared z21Socket;

    SOCKET webSocketListen, webSocketSend;
    SOCKET* webSocketListenPtr = &webSocketListen;
    SOCKET* webSocketSendPtr = &webSocketSend;

    winston::hal::UDPSocket::State webSocketState = { winston::hal::UDPSocket::State::NotConnected };

    const std::string z21IP = { "192.168.0.100" };
    const unsigned short z21Port = 5000;


};

int main()
{
    winston::hal::text("Hello from Winston!");

    //using Modelleisenbahn = MRS<RailwayWithSiding>;
    //using Modelleisenbahn = MRS<TimeSaverRailway
    //using Modelleisenbahn = MRS<Y2020Railway>;

    setStoragePath(MRS::name());

    // setup
    MRS mrs;
    mrs.setup();

    // and loop
    while (true)
        mrs.loop();
}