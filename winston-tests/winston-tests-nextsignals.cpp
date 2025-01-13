#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\libwinston\Signal.h"
#include "..\winston\railways.h"
#include "..\winston\LoDi_API.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(NextSignalTests)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }
        std::shared_ptr<Y2024Railway> railway;

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

        void createLocos(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed)
        {
            winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
            winston::Locomotive::ThrottleSpeedMap speedMap{ {0, 0}, {100, 1000}, {255, 2550} };
            locoShed.push_back(winston::Locomotive::make(locoCallbacks(signalPassed), 3, standardFunctions, winston::Position::nullPosition(), speedMap, "BR 114", 100, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting));
        }

    public:
        TEST_METHOD(SetupNextSignals)
        {
            TestSignalController testSignalController;
            auto signalTower = winston::SignalTower::make(locoShed);
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
            createSignals(testSignalController, railway, [](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
                {
                    return winston::State::Finished;
                });

            auto PBF1a = railway->track(Y2024RailwayTracks::PBF1a);
            auto Turnout1 = railway->track(Y2024RailwayTracks::Turnout1);
            auto Turnout2 = railway->track(Y2024RailwayTracks::Turnout2);
            auto Turnout3 = railway->track(Y2024RailwayTracks::Turnout3);
            auto B1 = railway->track(Y2024RailwayTracks::B1);
            auto B7 = railway->track(Y2024RailwayTracks::B7);
            auto PBF1 = railway->track(Y2024RailwayTracks::PBF1);

            auto B1_A = B1->signalGuarding(winston::Track::Connection::A);
            auto B1_B = B1->signalGuarding(winston::Track::Connection::B);
            auto B7_A = B7->signalGuarding(winston::Track::Connection::A);
            auto B7_B = B7->signalGuarding(winston::Track::Connection::B);
            auto PBF1_A = PBF1->signalGuarding(winston::Track::Connection::A);
            auto PBF1_B = PBF1->signalGuarding(winston::Track::Connection::B);
            auto PBF1a_A = PBF1a->signalGuarding(winston::Track::Connection::A);

            /*
            ==== A 5U |8 ==== B7 ==== 8| 5U B ==== A ==== Turnout1 ==== C ==== A |8 5U ==== PBF1 ==== 8| 5U B ==== A ==== Turnout2 ==== B ==== A |8 5U ==== PBF1a ==== 8| 5U Deadend
            */
            auto PBF1_Provider_A_Facing = PBF1->getNextSignalProvider(winston::Track::Connection::A, winston::Signal::Pass::Facing);
            auto PBF1_Provider_A_Backside = PBF1->getNextSignalProvider(winston::Track::Connection::A, winston::Signal::Pass::Backside);
            auto PBF1_Provider_B_Facing = PBF1->getNextSignalProvider(winston::Track::Connection::B, winston::Signal::Pass::Facing);
            auto PBF1_Provider_B_Backside = PBF1->getNextSignalProvider(winston::Track::Connection::B, winston::Signal::Pass::Backside);

            Assert::IsTrue(PBF1_Provider_A_Facing->nextTurnout->turnout == Turnout1);
            Assert::IsTrue(PBF1_Provider_A_Facing->distance == 0);
            Assert::IsTrue(PBF1_Provider_B_Facing->nextTurnout->turnout == Turnout2);
            Assert::IsTrue(PBF1_Provider_B_Facing->distance == 0);
            Assert::IsTrue(PBF1_Provider_A_Backside->nextTurnout->turnout == Turnout1);
            Assert::IsTrue(PBF1_Provider_A_Backside->distance == 0);
            Assert::IsTrue(PBF1_Provider_B_Backside->nextTurnout->turnout == Turnout2);
            Assert::IsTrue(PBF1_Provider_B_Backside->distance == 0);

            auto Turnout1_Provider_A_Facing = Turnout1->getNextSignalProvider(winston::Track::Connection::A, winston::Signal::Pass::Facing);
            auto Turnout1_Provider_A_Backside = Turnout1->getNextSignalProvider(winston::Track::Connection::A, winston::Signal::Pass::Backside);
            auto Turnout1_Provider_C_Facing = Turnout1->getNextSignalProvider(winston::Track::Connection::C, winston::Signal::Pass::Facing);
            auto Turnout1_Provider_C_Backside = Turnout1->getNextSignalProvider(winston::Track::Connection::C, winston::Signal::Pass::Backside);

            Assert::IsTrue(Turnout1_Provider_A_Facing->signal == B7_A);
            Assert::IsTrue(Turnout1_Provider_A_Facing->distance == B7->length() - B7_A->distance());
            Assert::IsTrue(Turnout1_Provider_A_Backside->signal == B7_B);
            Assert::IsTrue(Turnout1_Provider_A_Backside->distance == B7_B->distance());
            Assert::IsTrue(Turnout1_Provider_C_Facing->signal == PBF1_B);
            Assert::IsTrue(Turnout1_Provider_C_Facing->distance == PBF1->length() - PBF1_B->distance());
            Assert::IsTrue(Turnout1_Provider_C_Backside->signal == PBF1_A);
            Assert::IsTrue(Turnout1_Provider_C_Backside->distance == PBF1_A->distance());
        }
        TEST_METHOD(LocoNextSignals)
        {
            TestSignalController testSignalController;
            auto signalTower = winston::SignalTower::make(locoShed);
            railway = Y2024Railway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(railway->init() == winston::Result::OK);
            createSignals(testSignalController, railway, [](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
                {
                    return winston::State::Finished;
                });

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
            auto B1_B = B1->signalGuarding(winston::Track::Connection::B);
            auto B6_B = B6->signalGuarding(winston::Track::Connection::B);
            auto B7_A = B7->signalGuarding(winston::Track::Connection::A);
            auto B7_B = B7->signalGuarding(winston::Track::Connection::B);
            auto PBF1_A = PBF1->signalGuarding(winston::Track::Connection::A);
            auto PBF1_B = PBF1->signalGuarding(winston::Track::Connection::B);
            auto PBF1a_A = PBF1a->signalGuarding(winston::Track::Connection::A);

            Turnout1->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_B);

            /*
            
			==== A 5U |8 ==== B7 ==== 8| 5U B ==== A ==== Turnout1 ==== C ==== A |8 5U ==== PBF1 ==== 8| 5U B ==== A ==== Turnout2 ==== B ==== A |8 5U ==== PBF1a ==== 8| 5U Deadend

            */

            auto pos = winston::Position(PBF1, winston::Track::Connection::A, PBF1->length() / 2);
            loco->railOnto(pos);
            Assert::IsTrue(loco->isNextSignal(PBF1_A));
            Assert::IsTrue(loco->isNextSignal(PBF1_B));
            Assert::IsTrue(loco->isNextSignal(PBF1a_A));
            Assert::IsTrue(loco->isNextSignal(B7_B));

            pos = winston::Position(PBF1, winston::Track::Connection::A, 2);
            loco->railOnto(pos);
            Assert::IsTrue(loco->isNextSignal(B7_B));
            Assert::IsTrue(loco->isNextSignal(B7_A));
            Assert::IsTrue(loco->isNextSignal(PBF1_A));
            Assert::IsTrue(loco->isNextSignal(PBF1_B));
        }
    };
}
