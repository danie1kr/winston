#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	TEST_CLASS(MiniRailwayTest)
	{
        std::shared_ptr<MiniRailway> testRailway;
	public:

		TEST_METHOD(RailInit)
		{
            testRailway = MiniRailway::make();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
		}

        TEST_METHOD(RailTraverse)
        {
            testRailway = MiniRailway::make();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);

            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));
            
            winston::Section::Shared onto, onto2;

            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto.get() == t1.get());

            Assert::IsFalse(a->traverse(winston::Section::Connection::A, onto));
            Assert::IsTrue(onto == nullptr);

            t1->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == b.get());

            t1->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_directForcedToggleTurnout)
        {
            testRailway = MiniRailway::make();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            winston::SignalBox::Shared& signalBox = std::make_shared<winston::SignalBox>(std::dynamic_pointer_cast<winston::Railway>(testRailway), nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedForcedToggleTurnout)
        {
            testRailway = MiniRailway::make();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            winston::SignalBox::Shared& signalBox = std::make_shared<winston::SignalBox>(std::dynamic_pointer_cast<winston::Railway>(testRailway), nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->notify(winston::EventTurnoutFinalizeToggle::make(cb, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedExternallyAcknowledgedToggleTurnout)
        {
            testRailway = MiniRailway::make();
            Assert::IsTrue(testRailway->init() == winston::Result::Ok);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            winston::SignalBox::Shared& signalBox = std::make_shared<winston::SignalBox>(std::dynamic_pointer_cast<winston::Railway>(testRailway), nullMutex);

            auto direction = winston::Turnout::Direction::A_B;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == winston::Turnout::Direction::Changing);

            Assert::IsFalse(t1->traverse(winston::Section::Connection::A, onto));
            Assert::IsFalse(t1->traverse(winston::Section::Connection::B, onto));
            Assert::IsFalse(t1->traverse(winston::Section::Connection::C, onto));

            auto cb2 = std::make_shared<winston::Callback>([]() {});
            signalBox->notify(winston::EventTurnoutFinalizeToggle::make(cb2, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == b.get());
        }
	};
}
