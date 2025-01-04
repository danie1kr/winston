#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(DrivingMathTests)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }

        std::shared_ptr<Y2021Railway> railway;
        std::shared_ptr<SignalTestRailway> signalTestRailway;

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
            callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) -> const winston::Result
            {
                return winston::Result::OK;
            };

            callbacks.functions = [=](const winston::Address address, const uint32_t functions) -> const winston::Result
            {
                return winston::Result::OK;
            };

            callbacks.signalPassed = [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
            {
                return winston::Result::OK;
            };

            return callbacks;
        }
    public:
        TEST_METHOD(PositionMath)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // deadend-A = length of bumper
            auto PBF1a = railway->track(Y2021RailwayTracks::PBF1a);
            auto PBF1a_deadEnd = winston::Position(PBF1a, winston::Track::Connection::DeadEnd, 0);
            auto PBF1a_A = winston::Position(PBF1a, winston::Track::Connection::A, 0);
            auto A_to_deadEnd = PBF1a_A.minus(PBF1a_deadEnd);
            Assert::IsTrue(A_to_deadEnd == PBF1a->length());

            // N1_A_50 is 50mm before N1_A
            auto N1 = railway->track(Y2021RailwayTracks::N1);
            auto N1_A = winston::Position(N1, winston::Track::Connection::A, 0);
            auto N1_A_50 = winston::Position(N1, winston::Track::Connection::A, 50);
            Assert::IsTrue(N1_A.minus(N1_A_50) == 50);
            Assert::IsTrue(N1_A_50.minus(N1_A) == -50);
        }

        TEST_METHOD(DriveWithinTrack)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // start at dead end for 50mm
            auto PBF1a = railway->track(Y2021RailwayTracks::PBF1a);
            auto start_PBF1a_deadEnd = winston::Position(PBF1a, winston::Track::Connection::DeadEnd, 0);
            auto end = winston::Position(PBF1a, winston::Track::Connection::DeadEnd, 0);
            const winston::Distance distance = 50;
            auto transit = end.drive(distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result{ return winston::Result::OK; });
            Assert::IsTrue(transit == winston::Position::Transit::Stay);
            // end is +distance away from start (at dead end)
            Assert::IsTrue(start_PBF1a_deadEnd.minus(end) == distance);
            // start is -distance away from end 
            Assert::IsTrue(end.minus(start_PBF1a_deadEnd) == -distance);
        }

        TEST_METHOD(DriveCrossTrackRail2Rail)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // start at 80mm before PBF3 B and drive for 100mm onto B4, 20mm behind A
            auto PBF3a = railway->track(Y2021RailwayTracks::PBF3a);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto start_PBF3a_B = winston::Position(PBF3a, winston::Track::Connection::B, 80);
            auto end = winston::Position(PBF3a, winston::Track::Connection::B, 80);
            auto end_B4_A = winston::Position(B4, winston::Track::Connection::A, 20);
            const winston::Distance distance = 100;
            auto transit = end.drive(-distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(end_B4_A.trackName(), end.trackName());
            const auto travelled = start_PBF3a_B.minus(end);
            Assert::IsTrue(travelled == distance);
        }

        TEST_METHOD(DriveCrossTrackRail2RailOtherDirection)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // start at 80mm before PBF3 B and drive for 100mm onto B4, 20mm behind A
            auto PBF3a = railway->track(Y2021RailwayTracks::PBF3a);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto start_PBF3a_B = winston::Position(PBF3a, winston::Track::Connection::A, 80);
            auto end = winston::Position(PBF3a, winston::Track::Connection::A, 80);
            auto end_B4_A = winston::Position(B4, winston::Track::Connection::A, 20);
            const winston::Distance distance = 100 + PBF3a->length() - 80;
            auto transit = end.drive(distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(end_B4_A.trackName(), end.trackName());
            const auto travelled = start_PBF3a_B.minus(end);
            Assert::IsTrue(travelled == distance);
        }

        TEST_METHOD(DriveCrossTrackRailTurnoutRail)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout12 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout12));
            Turnout12->finalizeChangeTo(winston::Turnout::Direction::A_B);

            auto start = winston::Position(B6, winston::Track::Connection::A, 50);
            auto end = winston::Position(B6, winston::Track::Connection::A, 50);
            auto expect = winston::Position(B5, winston::Track::Connection::B, 80);

            auto distance = (signed)(50 + Turnout12->length() + 80);
            auto transit = end.drive(-distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });

            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(expect.trackName(), end.trackName());
            Assert::IsTrue(end.connection() == expect.connection());
            Assert::IsTrue(end.distance() == expect.distance());
        }

        TEST_METHOD(DriveCrossTrackRailTurnoutOpen)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout12 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout12));
            Turnout12->finalizeChangeTo(winston::Turnout::Direction::A_C);

            auto start = winston::Position(B6, winston::Track::Connection::A, 50);
            auto end = winston::Position(B6, winston::Track::Connection::A, 50);
            auto expect = winston::Position(B5, winston::Track::Connection::B, 80);

            auto distance = (signed)(50 + Turnout12->length() + 80);
            auto transit = end.drive(-distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });

            Assert::IsTrue(transit == winston::Position::Transit::TraversalError);
        }

        TEST_METHOD(DriveWholeLoop)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto PBF3 = railway->track(Y2021RailwayTracks::PBF3);
            auto Turnout6 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout6));
            auto PBF3a = railway->track(Y2021RailwayTracks::PBF3a);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto Turnout10 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout10));
            auto Turnout11 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout11));
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout12 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout12));
            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto Turnout2 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout2));
            auto Turnout3 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout3));

            Turnout6->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout10->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout11->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout12->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_B);

            auto pos = winston::Position(PBF3, winston::Track::Connection::A, 50);
            auto distance = (signed)(PBF3->length() + Turnout6->length() + PBF3a->length() + B4->length() + Turnout10->length() + Turnout11->length() + B5->length() + Turnout12->length() + B6->length() + Turnout2->length() + Turnout3->length() + 10);
            auto expect = winston::Position(PBF3, winston::Track::Connection::A, 60);

            auto transit = pos.drive(distance, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(expect.trackName(), pos.trackName());
            Assert::IsTrue(pos.connection() == expect.connection());
            Assert::IsTrue(pos.distance() == expect.distance());
        }

        TEST_METHOD(LocoSpeedCalculation)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto N1 = railway->track(Y2021RailwayTracks::N1);
            auto pos = winston::Position(N1, winston::Track::Connection::A, 200);

            winston::Locomotive::ThrottleSpeedMap map{ {0, 0},{255, 50} };
            winston::Locomotive::Functions functions{};
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 90, 0);

            auto div = 10;// 00;
            auto distance = 320; // mm
            auto delay = 8000; //ms
            auto expectedSpeed = distance * 1000 / delay;
            // ==> 320 / 8 ==> 40mm/s

            loco->drive<true>(true, 100);
            loco->speedTrap(0);
            winston::hal::delay(delay / div);
            loco->speedTrap(distance / div);

            loco->railOnto(pos);
            winston::hal::delay(100);   // ==> distance = 4mm
            loco->update();
            auto newPos = loco->position();
            Assert::AreEqual(pos.trackName(), newPos.trackName());
            Assert::IsTrue(pos.connection() == newPos.connection());
            Assert::IsTrue(abs(abs(pos.distance() - newPos.distance()) - (expectedSpeed / div)) <= 2);
        }

        TEST_METHOD(DriveLocoOnTrack)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);
            auto N1 = railway->track(Y2021RailwayTracks::N1);
            auto pos = winston::Position(N1, winston::Track::Connection::A, 200);

            winston::Locomotive::ThrottleSpeedMap map{ {0, 0}, {100, 100}, {255, 255} };
            winston::Locomotive::Functions functions = {};
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 90, 0);
            auto expectedDistance = 10;
            loco->drive<true>(true, 100);
            loco->railOnto(pos);
            winston::hal::delay(100);   // ==> distance = 10mm
            loco->update();
            auto newPos = loco->position();
            Assert::AreEqual(pos.trackName(), newPos.trackName());
            Assert::IsTrue(pos.connection() == newPos.connection());
            Assert::IsTrue(abs(abs(pos.distance() - newPos.distance()) - (expectedDistance)) <= 2);
        }
        
        TEST_METHOD(DriveLocoOnRail2Rail)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);
            auto PBF3a = railway->track(Y2021RailwayTracks::PBF3a);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto pos = winston::Position(PBF3a, winston::Track::Connection::B, 50);
            auto target = winston::Position(B4, winston::Track::Connection::A, 50);

            winston::Locomotive::ThrottleSpeedMap map{ {0, 0}, {100, 1000}, {255, 2550} };
            winston::Locomotive::Functions functions = {};
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 90, 0);
            auto throttle = 100;
            loco->drive<true>(false, throttle);
            loco->railOnto(pos);
            winston::hal::delay(100);   // ==> distance = 100mm
            winston::Duration timeOnTour = std::chrono::milliseconds(100);
            //auto newPos = loco->moved(timeOnTour);
            loco->update();
            auto newPos = loco->position();
            auto targetDistance = inMilliseconds(timeOnTour) * map[throttle] / 1000;
            Assert::AreEqual(target.trackName(), newPos.trackName());
            Assert::IsTrue(target.connection() == newPos.connection());
            Assert::IsTrue(abs(target.distance() - newPos.distance() <= 5));
        }

        TEST_METHOD(DriveLocoOnLoop)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto PBF3 = railway->track(Y2021RailwayTracks::PBF3);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto Turnout8 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout8));
            auto Turnout9 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout9));
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout10 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout10));
            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto Turnout2 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout2));
            auto Turnout3 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout3));

            Turnout8->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout9->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout10->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Turnout2->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Turnout3->finalizeChangeTo(winston::Turnout::Direction::A_B);

            auto pos = winston::Position(PBF3, winston::Track::Connection::A, 50);
            auto distance = (signed)(PBF3->length() + B4->length() + Turnout8->length() + Turnout9->length() + B5->length() + Turnout10->length() + B6->length() + Turnout2->length() + Turnout3->length() + 10);
            auto expect = winston::Position(PBF3, winston::Track::Connection::A, 60);

            winston::Locomotive::ThrottleSpeedMap map{ {0, 0}, {100, 500000}, {255, 2550} };
            winston::Locomotive::Functions functions = {};
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 90, 0);
            auto throttle = 100;
            loco->drive<true>(true, throttle);
            loco->railOnto(pos);
            winston::hal::delay(100);   // ==> distance = 50000mm
            winston::Duration timeOnTour = std::chrono::milliseconds(100);
            //auto newPos = loco->moved(timeOnTour);
            loco->update();
            auto newPos = loco->position();
            auto travelledDistance = inMilliseconds(timeOnTour) * map[throttle] / 1000;
            Assert::AreEqual(expect.trackName(), newPos.trackName());
            Assert::IsTrue(newPos.connection() == expect.connection());
            //Assert::IsTrue(newPos.distance() - (travelledDistance - distance) < 50);
        }



        TEST_METHOD(DriveCollectSignals)
        {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            signalTestRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(signalTestRailway->init() == winston::Result::OK);
            auto l0 = signalTestRailway->track(SignalTestRailway::Tracks::L0);
            auto l1 = signalTestRailway->track(SignalTestRailway::Tracks::L1);
            auto l2 = signalTestRailway->track(SignalTestRailway::Tracks::L2);
            auto l3 = signalTestRailway->track(SignalTestRailway::Tracks::L3);
            auto l4 = signalTestRailway->track(SignalTestRailway::Tracks::L4);
            auto l5 = signalTestRailway->track(SignalTestRailway::Tracks::L5);
            auto l6 = signalTestRailway->track(SignalTestRailway::Tracks::L6);
            auto l7 = signalTestRailway->track(SignalTestRailway::Tracks::L7);
            auto l8 = signalTestRailway->track(SignalTestRailway::Tracks::L8);
            auto sL0a = l0->signalGuarding(winston::Track::Connection::A);
            auto sL1b = l1->signalGuarding(winston::Track::Connection::B);
            auto sL4a = l4->signalGuarding(winston::Track::Connection::A);
            auto sL7a = l7->signalGuarding(winston::Track::Connection::A);
            auto sL8a = l8->signalGuarding(winston::Track::Connection::A);

            sL0a->aspect(winston::Signal::Aspect::Halt);
            sL1b->aspect(winston::Signal::Aspect::Halt);
            sL4a->aspect(winston::Signal::Aspect::Halt);
            sL7a->aspect(winston::Signal::Aspect::Halt);
            sL8a->aspect(winston::Signal::Aspect::Halt);

            // |====L0====KL0a=KL1b====L1====L2====L3====L4=KL4a====L5====L6====L7====KL7a=KL8a====L8====|
            signalTower->setSignalOn(*l4, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            Assert::IsTrue(sL0a->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sL1b->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL4a->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sL7a->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL8a->shows(winston::Signal::Aspect::Halt));

            signalTower->setSignalOn(*l1, winston::Track::Connection::B, winston::Signal::Aspect::Go);
            Assert::IsTrue(sL0a->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sL1b->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sL4a->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sL7a->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL8a->shows(winston::Signal::Aspect::ExpectGo));

            signalTower->setSignalOn(*l1, winston::Track::Connection::B, winston::Signal::Aspect::Halt);
            Assert::IsTrue(sL0a->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sL1b->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL4a->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sL7a->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL8a->shows(winston::Signal::Aspect::Halt));

            signalTower->setSignalOn(*l4, winston::Track::Connection::A, winston::Signal::Aspect::Halt);
            Assert::IsTrue(sL0a->shows(winston::Signal::Aspect::ExpectHalt));
            Assert::IsTrue(sL1b->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL4a->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL7a->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sL8a->shows(winston::Signal::Aspect::ExpectHalt));

            // find none
            {
                auto pos = winston::Position(l0, winston::Track::Connection::DeadEnd, 50);
                auto distance = 10;
                auto expect = winston::Position(l0, winston::Track::Connection::DeadEnd, 60);

                std::vector<winston::Signal::Shared> passedSignals;
                auto transit = pos.drive(distance, [&](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result {
                    auto signal = track->signalGuarding(connection);
                    passedSignals.push_back(signal);
                    return winston::Result::OK;
                    });
                Assert::IsTrue(transit == winston::Position::Transit::Stay);
                Assert::AreEqual(expect.trackName(), pos.trackName());
                Assert::IsTrue(pos.connection() == expect.connection());
                Assert::IsTrue(pos.distance() == expect.distance());

                Assert::IsTrue(passedSignals.empty());
            }

            // find sL0a on l0
            {
                auto pos = winston::Position(l0, winston::Track::Connection::DeadEnd, 50);
                auto distance = (signed)(l0->length());
                auto expect = winston::Position(l1, winston::Track::Connection::B, 50);

                std::vector<winston::Signal::Shared> passedSignals;
                auto transit = pos.drive(distance, [&](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result {
                    auto signal = track->signalGuarding(connection);
                    passedSignals.push_back(signal);
                    return winston::Result::OK;
                    });
                Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
                Assert::AreEqual(expect.trackName(), pos.trackName());
                Assert::IsTrue(pos.connection() == expect.connection());
                Assert::IsTrue(pos.distance() == expect.distance());

                Assert::IsTrue(passedSignals.size() == 2);
                Assert::IsTrue(std::find_if(passedSignals.begin(), passedSignals.end(),
                    [&](const winston::Signal::Shared& s)
                    {
                        return s == sL0a;
                    }) != passedSignals.end());
                Assert::IsTrue(std::find_if(passedSignals.begin(), passedSignals.end(),
                    [&](const winston::Signal::Shared& s)
                    {
                        return s == sL1b;
                    }) != passedSignals.end());
            }

            // find sL0a on l0, sL4a on l4 
            {
                auto pos = winston::Position(l0, winston::Track::Connection::DeadEnd, 50);
                auto distance = (signed)(l0->length() + l1->length() + l2->length() + l3->length() + l4->length() + l5->length());
                auto expect = winston::Position(l6, winston::Track::Connection::B, 50);

                std::vector<winston::Signal::Shared> passedSignals;
                auto transit = pos.drive(distance, [&](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result {
                    auto signal = track->signalGuarding(connection);
                    passedSignals.push_back(signal);
                    return winston::Result::OK;
                    });
                Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
                Assert::AreEqual(expect.trackName(), pos.trackName());
                Assert::IsTrue(pos.connection() == expect.connection());
                Assert::IsTrue(pos.distance() == expect.distance());

                Assert::IsTrue(passedSignals.size() == 3);
                Assert::IsTrue(std::find_if(passedSignals.begin(), passedSignals.end(),
                    [&](const winston::Signal::Shared& s)
                    {
                        return s == sL0a;
                    }) != passedSignals.end());
                Assert::IsTrue(std::find_if(passedSignals.begin(), passedSignals.end(),
                    [&](const winston::Signal::Shared& s)
                    {
                        return s == sL1b;
                    }) != passedSignals.end());
                Assert::IsTrue(std::find_if(passedSignals.begin(), passedSignals.end(),
                    [&](const winston::Signal::Shared& s)
                    {
                        return s == sL4a;
                    }) != passedSignals.end());
            }
        }
    };
}
