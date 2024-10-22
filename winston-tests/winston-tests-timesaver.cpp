#include "CppUnitTest.h"

#include "..\winston\railways.h"
#include "..\libwinston\Winston.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(TimeSaverRailwayTest)
    {
        std::shared_ptr<TimeSaverRailway> testRailway; 
        
        static winston::Railway::Callbacks railwayCallbacks()
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
            {
                return winston::State::Finished;
            };

            return callbacks;
        }
    public:

        TEST_METHOD(RailInit)
        {
            testRailway = std::make_shared<TimeSaverRailway>(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
        }

        TEST_METHOD(SignalTower_directForcedToggleTurnout)
        {
            testRailway = std::make_shared<TimeSaverRailway>(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto b = testRailway->track(TimeSaverRailway::Tracks::B);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(TimeSaverRailway::Tracks::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(TimeSaverRailway::Tracks::Turnout2));

            winston::Track::Shared onto, onto2;
            auto signalTower = winston::SignalTower::make();

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(b->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == t2.get());
        }

        /* no longer supported: TEST_METHOD(Event_MultiplesWithOneCallback)
        {
            int count = 0;
            auto cb = std::make_shared<winston::Callback>([&count]() { 
                ++count; 
            });

            testRailway = std::make_shared<TimeSaverRailway>(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(TimeSaverRailway::Tracks::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(TimeSaverRailway::Tracks::Turnout2));

            winston::Track::Shared onto, onto2;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalTower = winston::SignalTower::make(tr);

            //signalTower->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            //signalTower->notify(winston::EventTurnoutStartToggle::make(cb, t2));

            signalTower->order(winston::Command::make([t1](const TimePoint &created) -> const winston::State { return t1->startToggle(); }));
            signalTower->order(winston::Command::make([t2](const TimePoint &created) -> const winston::State { return t2->startToggle(); }));


            signalTower->work();
            signalTower->work();
            signalTower->work();
            signalTower->work();

            Assert::IsTrue(count == 1);
        }*/
    };
}
