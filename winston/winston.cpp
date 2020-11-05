// winston-simulator.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <functional>

#include "../libwinston/Winston.h"

#include "winston.h"
#include "winston-minnow.h"
#include "railways.h"
#include "external/central-z21/Z21.h"

class MRS : public winston::ModelRailwaySystem<RailwayWithSiding::Shared, RailwayWithSiding::AddressTranslator::Shared, Z21::Shared>
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
                signalBox->notify(std::make_unique<winston::EventTurnoutStartToggle>(cb, turnout));
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

            const size_t sizePerMessage = size_t(0.8f * WinstonMinnowBufferSize);
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

    winston::DigitalCentralStation::Callbacks z21Callbacks()
    {
        winston::DigitalCentralStation::Callbacks callbacks;

        // what to do when the digital central station updated a turnout
        callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction)
        {
            auto id = this->railway->sectionIndex(turnout);
            turnoutSendState(id, direction);
        };

        return callbacks;
    }

    // setup our model railway system
    void systemSetup() { 
        // the user defined railway and its address translator
        this->railway = RailwayWithSiding::make();
        this->addressTranslator = RailwayWithSiding::AddressTranslator::make(railway);

        // the internal signal box
        winston::Railway::Shared rw = std::dynamic_pointer_cast<winston::Railway>(railway);
        this->signalBox = winston::SignalBox::make(rw, nullMutex);

        // the system specific digital central station
        auto at = std::dynamic_pointer_cast<winston::DigitalCentralStation::AddressTranslator>(addressTranslator);
        this->digitalCentralStation = Z21::make(at, this->signalBox, z21Callbacks());

        // a debug injector
        this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(this->digitalCentralStation);
    };
    
    // accept new requests and loop over what the signal box has to do
    void systemLoop() { 
    
        SOCKET listenSock;
        SOCKET sock;
        SOCKET* listenSockPtr = &listenSock;
        SOCKET* sockPtr = &sock;
        minnowStart(listenSockPtr, sockPtr, &this->minnowWPH, &this->minnowCD, &this->minnowRD, &this->minnowServer, [this](struct RecData* o, struct ConnData* cd, const char* msg, JErr* e, JVal* v)->int { return this->minnow_manageMessage(o, cd, msg, e, v); }, [this](MST* mst, const char* path, FetchPageSend send)->int {return this->minnow_fetchPage(mst, path, send);  });
        int minnowAccepted = 0;
        while (true)
        {
            if (!minnowAccepted)
                minnowAccepted = minnowAccept(listenSockPtr, 50, sockPtr, this->minnowServer, this->minnowWPH);
            if (minnowAccepted == 1 && !minnowLoop(listenSockPtr, sockPtr, this->minnowWPH, this->minnowCD, this->minnowRD, this->minnowServer))
                break;
            this->signalBox->work();
        }

        minnowClose(listenSockPtr, sockPtr);
    };

    // minnow related
    WssProtocolHandshake* minnowWPH = nullptr;
    ConnData* minnowCD = nullptr;
    RecData* minnowRD = nullptr;
    MS* minnowServer = nullptr;
};

int main()
{
    winston::hal::text("Hello from Winston!");

    // setup
    MRS mrs;
    mrs.setup();

    // and loop
    while (true)
        mrs.loop();
}