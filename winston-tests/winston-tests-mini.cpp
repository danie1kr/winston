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

        static winston::Railway::Callbacks railwayCallbacksWithSignals(winston::SignalBox::Shared signalBox)
        {
            winston::Railway::Callbacks callbacks;

            callbacks.turnoutUpdateCallback = signalBox->injectTurnoutSignalHandling([=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
            {
                return winston::State::Finished;
            });

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

            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));
            
            winston::Track::Shared onto, onto2;

            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto.get() == t1.get());

            Assert::IsFalse(a->traverse(winston::Track::Connection::A, onto, false));
            Assert::IsTrue(onto == nullptr);

            t1->finalizeChangeTo(winston::Turnout::Direction::A_B);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == b.get());

            t1->finalizeChangeTo(winston::Turnout::Direction::A_C);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_directForcedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedForcedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            auto direction = winston::Turnout::Direction::A_C;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1, direction](const unsigned long long& created) -> const winston::State { return t1->finalizeChangeTo(direction);  }));
            //EventTurnoutFinalizeToggle::make(cb, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalBox_notifiedExternallyAcknowledgedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Shared onto, onto2;
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            auto direction = winston::Turnout::Direction::A_B;
            auto cb = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1](const unsigned long long& created) -> const winston::State { return t1->startToggle(); }));
            //signalBox->notify(winston::EventTurnoutStartToggle::make(cb, t1));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == winston::Turnout::Direction::Changing);

            Assert::IsFalse(t1->traverse(winston::Track::Connection::A, onto, false));
            Assert::IsFalse(t1->traverse(winston::Track::Connection::B, onto, false));
            Assert::IsFalse(t1->traverse(winston::Track::Connection::C, onto, false));

            auto cb2 = std::make_shared<winston::Callback>([]() {});
            signalBox->order(winston::Command::make([t1, direction](const unsigned long long& created) -> const winston::State { return t1->finalizeChangeTo(direction); }));
            //signalBox->notify(winston::EventTurnoutFinalizeToggle::make(cb2, t1, direction));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(t1->direction() == direction);
            
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == b.get());
        }

        TEST_METHOD(Signals_forTurnouts) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = MiniRailway::make(railwayCallbacksWithSignals(signalBox));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            auto sBA = b->signalGuarding(winston::Track::Connection::A);
            auto sCA = c->signalGuarding(winston::Track::Connection::A);
            Assert::IsTrue(sBA.operator bool() == true);
            Assert::IsTrue(sCA.operator bool() == true);

            signalBox->setSignalsFor(t1);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));

            signalBox->order(winston::Command::make([t1](const unsigned long long& created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_C); }));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Go));
            
            signalBox->order(winston::Command::make([t1](const unsigned long long& created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_B); }));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));
        }
	};
}
