#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	TEST_CLASS(MiniRailwayTest)
	{
        std::shared_ptr<MiniRailway> testRailway;

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
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
		}

        TEST_METHOD(RailTraverse)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);

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
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalBox = winston::SignalBox::make(tr, nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedForcedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalBox = winston::SignalBox::make(tr, nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1, direction](const unsigned long& created) -> const winston::State { return t1->finalizeChangeTo(direction);  }));
            //EventTurnoutFinalizeToggle::make(cb, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedExternallyAcknowledgedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->section(MiniRailway::Sections::A);
            auto b = testRailway->section(MiniRailway::Sections::B);
            auto c = testRailway->section(MiniRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(MiniRailway::Sections::Turnout1));

            winston::Section::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto tr = std::dynamic_pointer_cast<winston::Railway>(testRailway);
            auto signalBox = winston::SignalBox::make(tr, nullMutex);

            auto direction = winston::Turnout::Direction::A_B;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1](const unsigned long& created) -> const winston::State { return t1->startToggle(); }));
            //signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == winston::Turnout::Direction::Changing);

            Assert::IsFalse(t1->traverse(winston::Section::Connection::A, onto));
            Assert::IsFalse(t1->traverse(winston::Section::Connection::B, onto));
            Assert::IsFalse(t1->traverse(winston::Section::Connection::C, onto));

            auto cb2 = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1, direction](const unsigned long& created) -> const winston::State { return t1->finalizeChangeTo(direction); }));
            //signalBox->notify(winston::EventTurnoutFinalizeToggle::make(cb2, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            
            Assert::IsTrue(a->traverse(winston::Section::Connection::DeadEnd, onto));
            Assert::IsTrue(onto->traverse(winston::Section::Connection::A, onto2));
            Assert::IsTrue(onto2.get() == b.get());
        }
	};
}
