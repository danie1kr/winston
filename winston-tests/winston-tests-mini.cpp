#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	TEST_CLASS(MiniRailwayTest)
	{
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }

        std::shared_ptr<MiniRailway> testRailway;

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
            
            winston::Track::Const onto, onto2;

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

        TEST_METHOD(SignalTower_directForcedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Const onto, onto2;
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            auto direction = winston::Turnout::Direction::A_C;
            t1->finalizeChangeTo(direction);
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalTower_notifiedForcedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Const onto, onto2;
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            auto direction = winston::Turnout::Direction::A_C;
            signalTower->order(winston::Command::make([t1, direction](const winston::TimePoint &created) -> const winston::State { return t1->finalizeChangeTo(direction);  }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(t1->direction() == direction);
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == c.get());
        }

        TEST_METHOD(SignalTower_notifiedExternallyAcknowledgedToggleTurnout)
        {
            testRailway = MiniRailway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            winston::Track::Const onto, onto2;
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            auto direction = winston::Turnout::Direction::A_B;
            signalTower->order(winston::Command::make([t1](const winston::TimePoint &created) -> const winston::State { return t1->startToggle(); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(t1->direction() == winston::Turnout::Direction::Changing);

            Assert::IsFalse(t1->traverse(winston::Track::Connection::A, onto, false));
            Assert::IsFalse(t1->traverse(winston::Track::Connection::B, onto, false));
            Assert::IsFalse(t1->traverse(winston::Track::Connection::C, onto, false));

            signalTower->order(winston::Command::make([t1, direction](const winston::TimePoint &created) -> const winston::State { return t1->finalizeChangeTo(direction); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(t1->direction() == direction);
            
            Assert::IsTrue(a->traverse(winston::Track::Connection::DeadEnd, onto, false));
            Assert::IsTrue(onto->traverse(winston::Track::Connection::A, onto2, false));
            Assert::IsTrue(onto2.get() == b.get());
        }

        TEST_METHOD(Signals_forTurnouts) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = MiniRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(MiniRailway::Tracks::A);
            auto b = testRailway->track(MiniRailway::Tracks::B);
            auto c = testRailway->track(MiniRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(MiniRailway::Tracks::Turnout1));

            auto sBA = b->signalGuarding(winston::Track::Connection::A);
            auto sCA = c->signalGuarding(winston::Track::Connection::A);
            Assert::IsTrue(sBA.operator bool() == true);
            Assert::IsTrue(sCA.operator bool() == true);

            signalTower->setSignalsFor(*t1);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));

            signalTower->order(winston::Command::make([t1](const winston::TimePoint &created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_C); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Go));
            
            signalTower->order(winston::Command::make([t1](const winston::TimePoint &created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_B); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));
        }
	};
}
