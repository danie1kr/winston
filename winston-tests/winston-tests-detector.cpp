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

        static winston::Locomotive::Callbacks locoCallbacks(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed)
        {
            winston::Locomotive::Callbacks callbacks;
            callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) -> const winston::Result
                {
                    return winston::Result::OK;
                };

            callbacks.functions = [=](const winston::Address address, const uint32_t functions) -> const winston::Result
                {
                    return winston::Result::OK;
                };

            callbacks.signalPassed = signalPassed;

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
                [](const std::string detectorName, const winston::Locomotive::Shared loco, const bool forward, winston::Segment::Shared segment, const winston::Detector::Change change, const winston::TimePoint when) -> const winston::Result
                {
                    if (change == winston::Detector::Change::Entered)
                        loco->entered(segment, when);
                    else
                        loco->left(segment, when);
                    return winston::Result::NotImplemented;
                };
            callbacks.occupied =
                [](const std::string detectorName, winston::Segment::Shared segment, const winston::Detector::Change change, const winston::TimePoint when) -> const winston::Result
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

        void createLocos(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed)
        {
            winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
            winston::Locomotive::ThrottleSpeedMap speedMap{ {0, 0}, {100, 1000}, {255, 2550} };
            locoShed.push_back(winston::Locomotive::make(locoCallbacks(signalPassed), 3, standardFunctions, winston::Position::nullPosition(), speedMap, "BR 114", 100, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting));
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
            packet.push_back(detector / 8 + 1); // address
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
            createLocos([](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
                {
                    return winston::Result::OK;
                });

            auto loco = locoShed[0];
            auto PBF1a = railway->track(Y2024RailwayTracks::PBF1a);

            injectLoco(PBF1a->segment(), loco, true);
            detectorLoops();

            Assert::IsTrue(loco->position().trackIndex() == PBF1a->index);
        }
        TEST_METHOD(LocoAppearTraverseOverMultipleTracksToNextSegmentAndCollectSignals)
        {
            auto signalTower = winston::SignalTower::make();
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
            setupDetectors();
            Assert::IsTrue(loDiCommander->isReady());

            std::queue<winston::Signal::Shared> passedSignals;
            createLocos([&passedSignals, &signalTower](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
                {
                    auto signal = track->signalGuarding(connection);
                    passedSignals.push(signal);
                    signalTower->setSignalsForLocoPassing(track, connection, pass);
                    return winston::Result::OK;
                });

            auto loco = locoShed[0];
            auto PBF1a = railway->track(Y2024RailwayTracks::PBF1a);
            auto Turnout1 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout1));
            auto Turnout2 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout2));
            auto B7 = railway->track(Y2024RailwayTracks::B7);
            auto PBF1 = railway->track(Y2024RailwayTracks::PBF1);

            auto PBF1_A = PBF1->signalGuarding(winston::Track::Connection::A);
            auto PBF1_B = PBF1->signalGuarding(winston::Track::Connection::B);
            auto PBF1a_A = PBF1a->signalGuarding(winston::Track::Connection::A);

            Turnout1->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            auto distanceB7_T1 = Turnout1->length() + 10;
            auto distanceT1_PBF1a = PBF1->length() + Turnout2->length() + 10;

            auto throttle = 100;
            loco->drive<true>(true, throttle);

            injectLoco(B7->segment(), loco, true);
            detectorLoops();
            Assert::IsTrue(loco->position().trackIndex() == (*railway->segment(B7->segment())->tracks().begin())->index);
            // PBF1_A should be red as loco is on B7 or turnout1
            // PBF1a_A should be green as turnout is set correctly and track is empty
            // PBF1_B should be green as the track is empty

            loco->railOnto(winston::Position(B7, winston::Track::Connection::A, B7->length()));
            winston::hal::delay(distanceB7_T1);
            injectLoco(PBF1->segment(), loco, true);
            injectLoco(B7->segment(), loco, false);
            detectorLoops();
            loco->update();
            Assert::IsTrue(passedSignals.size() == 1);
            auto sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1_A);
            // loco passed PBF1_A, it should be green now (track is free, turnout is set correctly)
            // PBF1a_A should be green as turnout is set correctly and track is empty
            // PBF1_B should be red as the track is now occupied by loco
            Assert::IsTrue(loco->position().trackIndex() == PBF1->index);

            winston::hal::delay(distanceT1_PBF1a);
            injectLoco(PBF1a->segment(), loco, true);
            injectLoco(PBF1->segment(), loco, false);
            detectorLoops();
            loco->update();
            Assert::IsTrue(passedSignals.size() == 2);
            sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1_B);
            sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1a_A);
            // loco passed PBF1_A, it should be green now (track is free, turnout is set correctly)
            // PBF1a_A should be red as turnout is set correctly but track is occupied by loco
            // PBF1_B should be green as the track is now free again
            Assert::IsTrue(loco->position().trackIndex() == PBF1a->index);
        }

        TEST_METHOD(LocoDisappearCompletely)
        {
        }

        TEST_METHOD(LocoStopBeforeHaltSignal)
        {
        }
    };
}
