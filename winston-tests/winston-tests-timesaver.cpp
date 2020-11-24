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

        TEST_METHOD(SignalBox_directForcedToggleTurnout)
        {
            testRailway = std::make_shared<TimeSaverRailway>(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto b = testRailway->section(TimeSaverRailway::Sections::B);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout2));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalBox = winston::SignalBox::make(tr, nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(b->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
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
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout2));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalBox = winston::SignalBox::make(tr, nullMutex);

            //signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            //signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t2));

            signalBox->order(winston::Command::make([t1](const unsigned long& created) -> const winston::State { return t1->startToggle(); }));
            signalBox->order(winston::Command::make([t2](const unsigned long& created) -> const winston::State { return t2->startToggle(); }));


            signalBox->work();
            signalBox->work();
            signalBox->work();
            signalBox->work();

            Assert::IsTrue(count == 1);
        }*/
    };
}
