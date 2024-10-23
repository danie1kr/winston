#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"
#include "..\winston\LoDi_API.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(DetectorTests)
    {
        std::shared_ptr<Y2024Railway> railway;

        winston::hal::DebugSocket::Shared loDiSocket;
        LoDi::Shared loDi;
        winston::DetectorDevice::Shared loDiCommander;

        std::vector<winston::Locomotive::Shared> locoShed;

        static winston::Railway::Callbacks railwayCallbacks()
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = [=](winston::Turnout& turnout, const winston::Turnout::Direction direction) -> const winston::State
                {
                    return winston::State::Finished;
                };

            return callbacks;
        }

        static winston::Railway::Callbacks railwayCallbacksWithSignals(winston::SignalTower::Shared signalTower)
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = signalTower->injectTurnoutSignalHandling([=](winston::Turnout& turnout, const winston::Turnout::Direction direction) -> const winston::State
                {
                    return winston::State::Finished;
                });

            return callbacks;
        }

        static winston::Locomotive::Callbacks locoCallbacks()
        {
            winston::Locomotive::Callbacks callbacks;
            callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward)
                {
                };

            callbacks.functions = [=](const winston::Address address, const uint32_t functions)
                {
                };

            return callbacks;
        }

        void setupDetectors()
        {
            loDiSocket = LoDi::createLoDiDebugSocket();
            this->loDi = LoDi::make(this->loDiSocket);

            this->loDi->discover();

            unsigned int count = 0;
            while (count++ < 24)
            {
                this->loDi->loop();
                winston::hal::delay(50);
            }

            this->loDiCommander = this->loDi->createS88Commander();

            winston::DetectorDevice::PortSegmentMap portSegmentMap;
            for (auto const& [id, segment] : this->railway->segments())
                portSegmentMap.insert({ id, segment });

            winston::DetectorDevice::Callbacks callbacks;
            callbacks.change =
                [](const std::string detectorName, const winston::Locomotive::Shared loco, winston::Segment::Shared segment, const winston::Detector::Change change, const winston::TimePoint when) -> const winston::Result
                {
                    return winston::Result::NotImplemented;
                };
            callbacks.occupied =
                [](const std::string detectorName, winston::Segment::Shared segment, const winston::Detector::Callbacks::Change change) -> const winston::Result
                {
                    return winston::Result::NotImplemented;
                };
            callbacks.locoFromAddress =
                [&](const winston::Address address) -> winston::Locomotive::Shared
                {
                    if (auto it = std::find_if(locoShed.begin(), locoShed.end(), [address](winston::Locomotive::Shared& loco) -> bool
                        {
                            return loco->address() == address;
                        }); it != std::end(locoShed))
                            return *it;
                    else
                        return nullptr;
                };

            this->loDiCommander->init(portSegmentMap, callbacks);

            count = 0;
            while (!loDiCommander->isReady() && count++ < 24)
            {
                this->loDi->loop();
                winston::hal::delay(50);
            }
        }

        void createLocos()
        {
            winston::Position pos(this->railway->track(Y2024RailwayTracks::B1), winston::Track::Connection::A, 100);
            winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
            locoShed.push_back(winston::Locomotive::make(locoCallbacks(), 3, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 114", 164, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting));
        }

        void injectLoco(uint8_t detector, winston::Locomotive::Shared loco, const bool enter)
        {
            winston::hal::DebugSocket::Packet packet;
            packet.push_back(0);     // size
            packet.push_back(9);     // size
            packet.push_back((unsigned char)LoDi::API::PacketType::EVT);  // ACK
            packet.push_back((unsigned char)LoDi::API::Event::S88LokAddrEvent);  // S88LokAddrEvent
            packet.push_back(1);  // Packet number
            packet.push_back(1);  // count
            packet.push_back(detector / 8); // address
            packet.push_back(detector % 8); // channel
            packet.push_back(loco->address() >> 8); // railcom address high
            packet.push_back(loco->address() & 0xFF); // railcom address low
            packet.push_back(enter ? 1 : 0); // status
            packet[1] = (unsigned char)(packet.size() - 2);// update size
            loDiSocket->addRecvPacket(packet);
        }

        void detectorLoops(const size_t count = 8)
        {
            for(size_t i = 0; i < count; ++i)
            {
                this->loDi->loop();
                winston::hal::delay(50);
            }
        }
    public:
        TEST_METHOD(DetectorSetup)
        {
            railway = Y2024Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);
            setupDetectors();
            Assert::IsTrue(loDiCommander->isReady());
        }
        TEST_METHOD(LocoAppear)
        {
            railway = Y2024Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);
            setupDetectors();
            Assert::IsTrue(loDiCommander->isReady());
            createLocos();

            injectLoco(13, locoShed[0], true);
            detectorLoops();
        }

        TEST_METHOD(LocoDisappear)
        {
        }

        TEST_METHOD(LocoDriveOnSegment)
        {
        }

        TEST_METHOD(LocoDriveToOtherSegment)
        {
        }

        TEST_METHOD(LocoPassSignal)
        {
        }
    };
}
