#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\libwinston\Signal.h"
#include "..\winston\railways.h"
#include "..\winston\LoDi_API.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(DetectorTests)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }
        std::shared_ptr<Y2024Railway> railway;

        winston::hal::DebugSocket::Shared loDiSocket;
        LoDi::Shared loDi;
        winston::DetectorDevice::Shared loDiCommander;

        std::vector<winston::Locomotive::Shared> locoShed;

        class TestSignalController : public winston::SignalController
        {
        public:
            TestSignalController()
                : SignalController(0, {})
            {
            };

            virtual ~TestSignalController() = default;

            template<class _Signal>
            const winston::Result attach(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback)
            {
                auto s = _Signal::make(0, [track, connection, signalUpdateCallback](const winston::Signal::Aspects aspect)->const winston::State {
                    return signalUpdateCallback(*track, connection, aspect);
                    }
                , distance);
                track->attachSignal(s, connection);
                return winston::Result::OK;
            }
        };

        void createSignals(TestSignalController& signalController, Y2024Railway::Shared railway, winston::Railway::Callbacks::SignalUpdateCallback signalUpdateCallback)
        {
            auto signalUpdateAlwaysHalt = [=](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
                {
                    return winston::State::Finished;
                };

            // Pbf
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::PBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::PBF1), winston::Track::Connection::B, 5U, signalUpdateCallback);

            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::PBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::PBF2), winston::Track::Connection::B, 5U, signalUpdateCallback);

            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::PBF1a), winston::Track::Connection::A, 5U, signalUpdateCallback);

            // N + LS
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::N1), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::N2), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::N3), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::N4), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::N5), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::LS1), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::LS2), winston::Track::Connection::A, 5U, signalUpdateCallback);

            // Track
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::B1), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B1), winston::Track::Connection::B, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B2), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B2), winston::Track::Connection::B, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B3), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B3), winston::Track::Connection::B, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B4), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B4), winston::Track::Connection::B, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B6), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B6), winston::Track::Connection::B, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::B7), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::B7), winston::Track::Connection::B, 5U, signalUpdateCallback);

            // leaving inner tracks
            signalController.attach<winston::SignalKS>(railway->track(Y2024Railway::Tracks::Z1), winston::Track::Connection::A, 5U, signalUpdateCallback);
            signalController.attach<winston::SignalH>(railway->track(Y2024Railway::Tracks::Z3), winston::Track::Connection::A, 5U, signalUpdateCallback);

            // track end bumper signals
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::N1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::N2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::N3), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::N4), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::N5), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::LS1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::LS2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
            signalController.attach<winston::SignalAlwaysHalt>(railway->track(Y2024Railway::Tracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
        
            railway->eachTrack([this](const Y2024Railway::Tracks tracksId, winston::Track::Shared track) {
                track->eachConnection([this, track](winston::Track& unused, const winston::Track::Connection connection) {
                    if (connection != winston::Track::Connection::DeadEnd)
                    {
                        winston::SignalTower::setupNextSignal(track, connection, winston::Signal::Pass::Facing);
                        winston::SignalTower::setupNextSignal(track, connection, winston::Signal::Pass::Backside);
                    }
                    });
                });
        };

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
                    return winston::Result::OK;
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

        void createLocos(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed, winston::Locomotive::ThrottleSpeedMap speedMap = { {0, 0}, {100, 1000}, {255, 2550} })
        {
            winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
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
            TestSignalController testSignalController;
            auto signalTower = winston::SignalTower::make(locoShed);
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
			createSignals(testSignalController, railway, [](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
				{
					return winston::State::Finished;
				});
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
            auto Turnout3 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout3));
            auto B1 = railway->track(Y2024RailwayTracks::B1);
            auto B6 = railway->track(Y2024RailwayTracks::B6);
            auto B7 = railway->track(Y2024RailwayTracks::B7);
            auto PBF1 = railway->track(Y2024RailwayTracks::PBF1);

            auto B1_A = B1->signalGuarding(winston::Track::Connection::A);
            auto B6_A = B6->signalGuarding(winston::Track::Connection::A);
            auto B6_B = B6->signalGuarding(winston::Track::Connection::B);
            auto B7_A = B7->signalGuarding(winston::Track::Connection::A);
            auto B7_B = B7->signalGuarding(winston::Track::Connection::B);
            auto PBF1_A = PBF1->signalGuarding(winston::Track::Connection::A);
            auto PBF1_B = PBF1->signalGuarding(winston::Track::Connection::B);
            auto PBF1a_A = PBF1a->signalGuarding(winston::Track::Connection::A);

            Turnout1->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            signalTower->setSignalsFor(*Turnout1);
            signalTower->setSignalsFor(*Turnout2);
            signalTower->setSignalsFor(*Turnout3);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            auto distanceB7_T1 = (Turnout1->length() + 1) * 10;
            auto distanceT1_PBF1a = (PBF1->length() + Turnout2->length() + 1) * 10;

            auto throttle = 10;
            loco->drive<true>(true, throttle);

            injectLoco(B7->segment(), loco, true);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            detectorLoops();
            Assert::IsTrue(loco->isNextSignal(PBF1_A));
            Assert::IsTrue(loco->isNextSignal(PBF1_B));
            Assert::IsTrue(loco->isNextSignal(B7_A));
            Assert::IsTrue(loco->isNextSignal(B7_B));
            Assert::IsTrue(loco->position().trackIndex() == (*railway->segment(B7->segment())->tracks().begin())->index);
            // we don't know yet, as the loco just appeared. Do not set signals yet
        
            loco->railOnto(winston::Position(B7, winston::Track::Connection::A, B7->length()));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            // and now we know:
            // PBF1_A should be red as loco is on B7
            // B7_B should be red as loco is on B7 but behind B7_B
            // PBF1a_A should be green as turnout is set correctly and track PBF1 is empty
            // PBF1_B should be green as the track PBF1a_A is empty
            Assert::IsTrue(loco->isNextSignal(PBF1_A));
            Assert::IsTrue(loco->isNextSignal(PBF1_B));
            Assert::IsTrue(loco->isNextSignal(B7_A));
            Assert::IsTrue(loco->isNextSignal(B7_B));

            Assert::IsTrue(B7_B->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(PBF1_A->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(PBF1a_A->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(PBF1_B->shows(winston::Signal::Aspect::Go));
            
            winston::hal::delay(distanceB7_T1);
            injectLoco(PBF1->segment(), loco, true);
            injectLoco(B7->segment(), loco, false);
            detectorLoops();
            loco->update();
            Assert::IsTrue(loco->position().trackIndex() == PBF1->index);
            Assert::IsTrue(passedSignals.size() == 1);
            auto sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1_A);
            // loco passed PBF1_A, it should be green now (track is free, turnout is set correctly, loco left the block)
            // PBF1a_A should be red as turnout is set correctly but track is not empty
            // PBF1_B should be green as the block behind is free
            Assert::IsTrue(PBF1_A->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(PBF1a_A->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(PBF1_B->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(B1_A->shows(winston::Signal::Aspect::Go));

            // toggle turnout 2 and 3 so B1 now enters PBF1. B1_a now should show red as it leads to the occupied section
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_C);
            signalTower->setSignalsFor(*Turnout2);
            signalTower->setSignalsFor(*Turnout3);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(B1_A->shows(winston::Signal::Aspect::Halt));

            // toggle turnout 2 and 3 so PBF1 now leads to PBF1a again. B1_a now should show green again as it leads to the occupied section
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_B);
            signalTower->setSignalsFor(*Turnout2);
            signalTower->setSignalsFor(*Turnout3);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(B1_A->shows(winston::Signal::Aspect::Go));

            winston::hal::delay(distanceT1_PBF1a);
            injectLoco(PBF1a->segment(), loco, true);
            injectLoco(PBF1->segment(), loco, false);
            detectorLoops();
            loco->update();
            Assert::IsTrue(loco->position().trackIndex() == PBF1a->index);
            Assert::IsTrue(passedSignals.size() == 2);
            sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1_B);
            sig = passedSignals.front(); passedSignals.pop(); Assert::IsTrue(sig == PBF1a_A);
            // loco passed PBF1_B and PBF1a_A
            // PBF1a_A should be green as turnout is set correctly and track empty
            // PBF1_B should be red as the track is occupied by loco
            Assert::IsTrue(PBF1_A->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(PBF1a_A->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(PBF1_B->shows(winston::Signal::Aspect::Halt));

            // toggle Turnout 2 away from PBF1a, so it should be green again
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_C);
            signalTower->setSignalsFor(*Turnout2);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(PBF1_B->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(B1_A->shows(winston::Signal::Aspect::Go));
        }

        TEST_METHOD(LocoDisappearCompletely)
        {
        }

        TEST_METHOD(LocoDriveARound)
        {
            TestSignalController testSignalController;
            auto signalTower = winston::SignalTower::make(locoShed);
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
            createSignals(testSignalController, railway, [](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
                {
                    return winston::State::Finished;
                });
            setupDetectors();
            Assert::IsTrue(loDiCommander->isReady());

            createLocos([&signalTower](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
                {
                    auto signal = track->signalGuarding(connection);
                    signalTower->setSignalsForLocoPassing(track, connection, pass);
                    return winston::Result::OK;
                },
                { {0, 0}, {100, 10}, {255, 25} });

            auto loco = locoShed[0];
            auto PBF1a = railway->track(Y2024RailwayTracks::PBF1a);
            auto B7 = railway->track(Y2024RailwayTracks::B7);
            auto Turnout1 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout1));
            auto Turnout2 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout2));
            auto Turnout3 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout3));
            auto Turnout4 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout4));
            auto Turnout5 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout5));
            auto Turnout6 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout6));
            auto Turnout7 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout7));

            Turnout1->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout4->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout5->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout6->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout7->finalizeChangeTo(winston::Turnout::Direction::A_B);

            signalTower->setSignalsFor(*Turnout1);
            signalTower->setSignalsFor(*Turnout2);
            signalTower->setSignalsFor(*Turnout3);
            signalTower->setSignalsFor(*Turnout4);
            signalTower->setSignalsFor(*Turnout5);
            signalTower->setSignalsFor(*Turnout6);
            signalTower->setSignalsFor(*Turnout7);

            for (int i = 0; i < 80; ++i)
                signalTower->loop();

            auto throttle = 100;
            loco->drive<true>(true, throttle);

            auto PBF2 = railway->track(Y2024RailwayTracks::PBF2);
            loco->railOnto(winston::Position(PBF2, winston::Track::Connection::A, 50));
            injectLoco(PBF2->segment(), loco, true);
            detectorLoops();

            size_t expectedTrackListIndex = 0;
            std::array<winston::Track::Const, 12> expectedTrackList = {
                    railway->track(Y2024RailwayTracks::Turnout3),
                    railway->track(Y2024RailwayTracks::B1),
                    railway->track(Y2024RailwayTracks::B2),
                    railway->track(Y2024RailwayTracks::Turnout4),
                    railway->track(Y2024RailwayTracks::B3),
                    railway->track(Y2024RailwayTracks::Turnout6),
                    railway->track(Y2024RailwayTracks::B5),
                    railway->track(Y2024RailwayTracks::Turnout7),
                    railway->track(Y2024RailwayTracks::B6),
                    railway->track(Y2024RailwayTracks::B7),
                    railway->track(Y2024RailwayTracks::Turnout1),
                    railway->track(Y2024RailwayTracks::PBF2)
            };

            bool back = false;
            while (!back)
            {
                auto oldLocoTrack = loco->position().track();
                winston::Position::Transit transit = winston::Position::Transit::Stay;
                while (transit == winston::Position::Transit::Stay)
                {
                    loco->update(transit);
                    winston::hal::delay(100);
                }
                Assert::IsTrue(transit != winston::Position::Transit::TraversalError);
                auto locoTrack = loco->position().track();
                Assert::IsTrue(expectedTrackListIndex < expectedTrackList.size());
                Assert::IsTrue(locoTrack == expectedTrackList[expectedTrackListIndex++]);
                injectLoco(locoTrack->segment(), loco, true);
                injectLoco(oldLocoTrack->segment(), loco, false);
                detectorLoops();

                back = loco->position().track() == PBF2 && loco->position().distance() < 50;
            }
        }

        TEST_METHOD(LocoStopBeforeHaltSignal)
        {
            TestSignalController testSignalController;
            auto signalTower = winston::SignalTower::make(locoShed);
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
            createSignals(testSignalController, railway, [](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
                {
                    return winston::State::Finished;
                });
            setupDetectors();
            Assert::IsTrue(loDiCommander->isReady());

            createLocos([&signalTower](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
                {
                    auto signal = track->signalGuarding(connection);
                    signalTower->setSignalsForLocoPassing(track, connection, pass);
                    return winston::Result::OK;
                },
                { {0, 0}, {100, 100}, {255, 255} });

            auto loco = locoShed[0];
            loco->autodrive(true, false);
            auto B3 = railway->track(Y2024RailwayTracks::B3);
            auto Turnout1 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout1));
            auto Turnout2 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout2));
            auto Turnout3 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout3));
            auto Turnout4 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout4));
            auto Turnout5 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout5));
            auto Turnout6 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout6));
            auto Turnout7 = std::dynamic_pointer_cast<winston::Turnout>(railway->track(Y2024RailwayTracks::Turnout7));

            Turnout1->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout4->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout5->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout6->finalizeChangeTo(winston::Turnout::Direction::A_C); // open Turnout6 to A_C so the loco needs to stop at B3_B
            Turnout7->finalizeChangeTo(winston::Turnout::Direction::A_B);

            signalTower->setSignalsFor(*Turnout1);
            signalTower->setSignalsFor(*Turnout2);
            signalTower->setSignalsFor(*Turnout3);
            signalTower->setSignalsFor(*Turnout4);
            signalTower->setSignalsFor(*Turnout5);
            signalTower->setSignalsFor(*Turnout6);
            signalTower->setSignalsFor(*Turnout7);

            for (int i = 0; i < 24; ++i)
                signalTower->loop();

            auto throttle = 100;
            loco->drive<true>(true, throttle);

            auto PBF2 = railway->track(Y2024RailwayTracks::PBF2);
            loco->railOnto(winston::Position(PBF2, winston::Track::Connection::A, 50));
            injectLoco(PBF2->segment(), loco, true);
            detectorLoops();

            size_t expectedTrackListIndex = 0;
            std::array<winston::Track::Const, 5> expectedTrackList = {
                    railway->track(Y2024RailwayTracks::Turnout3),
                    railway->track(Y2024RailwayTracks::B1),
                    railway->track(Y2024RailwayTracks::B2),
                    railway->track(Y2024RailwayTracks::Turnout4),
                    railway->track(Y2024RailwayTracks::B3)
            };

            bool done = false;
            bool completed = false;
            while (!done)
            {
                auto oldLocoTrack = loco->position().track();
                winston::Position::Transit transit = winston::Position::Transit::Stay;
                while (transit == winston::Position::Transit::Stay && loco->speed() != 0)
                {
                    loco->update(transit);
                    winston::hal::delay(100);
                }
                Assert::IsTrue(transit != winston::Position::Transit::TraversalError);
                auto locoTrack = loco->position().track();
                Assert::IsTrue(expectedTrackListIndex <= expectedTrackList.size());
                if(expectedTrackListIndex < expectedTrackList.size())
                { 
                    Assert::IsTrue(locoTrack == expectedTrackList[expectedTrackListIndex++]);
                    injectLoco(locoTrack->segment(), loco, true);
                    injectLoco(oldLocoTrack->segment(), loco, false);
                    detectorLoops();
                }
                else if (expectedTrackListIndex == expectedTrackList.size())
                {
                    Assert::IsTrue(locoTrack == expectedTrackList[expectedTrackListIndex - 1]);
                    Assert::IsTrue(loco->speed() == 0);
                    auto nextSignal = loco->details.nextSignals.get(true, winston::Signal::Pass::Facing);
                    Assert::IsTrue(nextSignal->distance > 10 && nextSignal->distance < 20);
                    completed = true;
                }
                done = loco->speed() == 0;
            }
            Assert::IsTrue(completed);
        }
    };
}
