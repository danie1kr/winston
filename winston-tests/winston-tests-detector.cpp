#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(DetectorTests)
    {
        std::shared_ptr<Y2021Railway> railway;

        static winston::Railway::Callbacks railwayCallbacks()
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
            {
                return winston::State::Finished;
            };

            return callbacks;
        }

        static winston::Railway::Callbacks railwayCallbacksWithSignals(winston::SignalTower::Shared signalTower)
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = signalTower->injectTurnoutSignalHandling([=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
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
            auto transit = end.drive(distance);
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
            auto PBF3 = railway->track(Y2021RailwayTracks::PBF3);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto start_PBF3_B = winston::Position(PBF3, winston::Track::Connection::B, 80);
            auto end = winston::Position(PBF3, winston::Track::Connection::B, 80);
            auto end_B4_A = winston::Position(B4, winston::Track::Connection::A, 20);
            const winston::Distance distance = 100;
            auto transit = end.drive(-distance);
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(end.trackName(), end_B4_A.trackName());
            const auto travelled = start_PBF3_B.minus(end);
            Assert::IsTrue(travelled == distance);
        }

        TEST_METHOD(DriveCrossTrackRail2RailOtherDirection)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // start at 80mm before PBF3 B and drive for 100mm onto B4, 20mm behind A
            auto PBF3 = railway->track(Y2021RailwayTracks::PBF3);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto start_PBF3_B = winston::Position(PBF3, winston::Track::Connection::A, 80);
            auto end = winston::Position(PBF3, winston::Track::Connection::A, 80);
            auto end_B4_A = winston::Position(B4, winston::Track::Connection::A, 20);
            const winston::Distance distance = 100 + PBF3->length() - 80;
            auto transit = end.drive(distance);
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(end.trackName(), end_B4_A.trackName());
            const auto travelled = start_PBF3_B.minus(end);
            Assert::IsTrue(travelled == distance);
        }

        TEST_METHOD(DriveCrossTrackRailTurnoutRail)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout10 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout10));
            Turnout10->finalizeChangeTo(winston::Turnout::Direction::A_B);

            auto start = winston::Position(B6, winston::Track::Connection::A, 50);
            auto end = winston::Position(B6, winston::Track::Connection::A, 50);
            auto expect = winston::Position(B5, winston::Track::Connection::B, 80);

            auto distance = (signed)(50 + Turnout10->length() + 80);
            auto transit = end.drive(-distance);

            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(end.trackName(), expect.trackName());
            Assert::IsTrue(end.connection() == expect.connection());
            Assert::IsTrue(end.distance() == expect.distance());
        }

        TEST_METHOD(DriveCrossTrackRailTurnoutOpen)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            auto B6 = railway->track(Y2021RailwayTracks::B6);
            auto B5 = railway->track(Y2021RailwayTracks::B5);
            auto Turnout10 = std::static_pointer_cast<winston::Turnout>(railway->track(Y2021RailwayTracks::Turnout10));
            Turnout10->finalizeChangeTo(winston::Turnout::Direction::A_C);

            auto start = winston::Position(B6, winston::Track::Connection::A, 50);
            auto end = winston::Position(B6, winston::Track::Connection::A, 50);
            auto expect = winston::Position(B5, winston::Track::Connection::B, 80);

            auto distance = (signed)(50 + Turnout10->length() + 80);
            auto transit = end.drive(-distance);

            Assert::IsTrue(transit == winston::Position::Transit::TraversalError);
        }

        TEST_METHOD(DriveWholeLoop)
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

            auto transit = pos.drive(distance);
            Assert::IsTrue(transit == winston::Position::Transit::CrossTrack);
            Assert::AreEqual(pos.trackName(), expect.trackName());
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
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 0);

            auto div = 10;// 00;
            auto distance = 320; // mm
            auto delay = 8000; //ms
            auto expectedSpeed = distance * 1000 / delay;
            // ==> 320 / 8 ==> 40mm/s

            loco->drive<true>(true, 100);
            loco->speedTrap(0);
            winston::hal::delay(delay / div);
            loco->speedTrap(distance / div);

            loco->position(pos);
            winston::hal::delay(100);   // ==> distance = 4mm
            winston::Duration timeOnTour;
            auto newPos = loco->moved(timeOnTour);
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
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 0);
            auto expectedDistance = 10;
            loco->drive<true>(true, 100);
            loco->position(pos);
            winston::hal::delay(100);   // ==> distance = 10mm
            winston::Duration timeOnTour;
            auto newPos = loco->moved(timeOnTour);
            Assert::AreEqual(pos.trackName(), newPos.trackName());
            Assert::IsTrue(pos.connection() == newPos.connection());
            Assert::IsTrue(abs(abs(pos.distance() - newPos.distance()) - (expectedDistance)) <= 2);
        }
        
        TEST_METHOD(DriveLocoOnRail2Rail)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);
            auto PBF3 = railway->track(Y2021RailwayTracks::PBF3);
            auto B4 = railway->track(Y2021RailwayTracks::B4);
            auto pos = winston::Position(PBF3, winston::Track::Connection::B, 50);
            auto target = winston::Position(B4, winston::Track::Connection::A, 50);

            winston::Locomotive::ThrottleSpeedMap map{ {0, 0}, {100, 1000}, {255, 2550} };
            winston::Locomotive::Functions functions = {};
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 0);
            auto throttle = 100;
            loco->drive<true>(false, throttle);
            loco->position(pos);
            winston::hal::delay(100);   // ==> distance = 100mm
            winston::Duration timeOnTour;
            auto newPos = loco->moved(timeOnTour);
            auto targetDistance = inMilliseconds(timeOnTour) * map[throttle] / 1000;
            Assert::AreEqual(target.trackName(), newPos.trackName());
            Assert::IsTrue(target.connection() == newPos.connection());
            Assert::IsTrue(newPos.distance() + target.distance() == targetDistance);
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
            auto loco = winston::Locomotive::make(locoCallbacks(), 0, functions, pos, map, "testloco1", 0);
            auto throttle = 100;
            loco->drive<true>(true, throttle);
            loco->position(pos);
            winston::hal::delay(100);   // ==> distance = 50000mm
            winston::Duration timeOnTour;
            auto& newPos = loco->moved(timeOnTour);
            auto travelledDistance = inMilliseconds(timeOnTour) * map[throttle] / 1000;
            Assert::AreEqual(expect.trackName(), newPos.trackName());
            Assert::IsTrue(newPos.connection() == expect.connection());
            //Assert::IsTrue(newPos.distance() - (travelledDistance - distance) < 50);
        }
    };
}
