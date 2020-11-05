#include "CppUnitTest.h"

#include "..\winston\railways.h"
#include "..\libwinston\Winston.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(TimeSaverRailwayTest)
    {
        std::shared_ptr<TimeSaverRailway> testRailway;
    public:

        TEST_METHOD(RailInit)
        {
            testRailway = std::make_shared<TimeSaverRailway>();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
        }

        TEST_METHOD(SignalBox_directForcedToggleTurnout)
        {
            testRailway = std::make_shared<TimeSaverRailway>();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
            auto b = testRailway->section(TimeSaverRailway::Sections::B);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout2));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            winston::SignalBox::Shared& signalBox = std::make_shared<winston::SignalBox>(std::dynamic_pointer_cast<winston::Railway>(testRailway), nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(b->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == t2.get());
        }

        TEST_METHOD(Event_MultiplesWithOneCallback)
        {
            int count = 0;
            auto cb = std::make_shared<winston::Callback>([&count]() { 
                ++count; 
            });

            testRailway = std::make_shared<TimeSaverRailway>();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout1));
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(TimeSaverRailway::Sections::Turnout2));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            winston::SignalBox::Shared& signalBox = std::make_shared<winston::SignalBox>(std::dynamic_pointer_cast<winston::Railway>(testRailway), nullMutex);

            signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t2));

            signalBox->work();
            signalBox->work();
            signalBox->work();
            signalBox->work();

            Assert::IsTrue(count == 1);
        }
    };
}
