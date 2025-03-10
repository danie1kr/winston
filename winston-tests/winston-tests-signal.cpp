#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(SignalRailwayTest)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }

        std::shared_ptr<SignalTestRailway> testRailway;

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
        TEST_METHOD(Signals_forTurnouts) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(SignalTestRailway::Tracks::A);
            auto b = testRailway->track(SignalTestRailway::Tracks::B);
            auto c = testRailway->track(SignalTestRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(SignalTestRailway::Tracks::Turnout1));

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

        TEST_METHOD(Signals_longTrackInBetween) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto l0 = testRailway->track(SignalTestRailway::Tracks::L0);
            auto l1 = testRailway->track(SignalTestRailway::Tracks::L1);
            auto l2 = testRailway->track(SignalTestRailway::Tracks::L2);
            auto l3 = testRailway->track(SignalTestRailway::Tracks::L3);
            auto l4 = testRailway->track(SignalTestRailway::Tracks::L4);
            auto l5 = testRailway->track(SignalTestRailway::Tracks::L5);
            auto l6 = testRailway->track(SignalTestRailway::Tracks::L6);
            auto l7 = testRailway->track(SignalTestRailway::Tracks::L7);
            auto l8 = testRailway->track(SignalTestRailway::Tracks::L8);
            auto sL0a = l0->signalGuarding(winston::Track::Connection::A);
            auto sL1b = l1->signalGuarding(winston::Track::Connection::B);
            auto sL4a = l4->signalGuarding(winston::Track::Connection::A);
            auto sL7a = l7 ->signalGuarding(winston::Track::Connection::A);
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
        }

        TEST_METHOD(Signals_fullTurnoutsSignalization) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto r = testRailway->track(SignalTestRailway::Tracks::R);
            auto s = testRailway->track(SignalTestRailway::Tracks::S);
            auto t = testRailway->track(SignalTestRailway::Tracks::T);
            auto u = testRailway->track(SignalTestRailway::Tracks::U);
            auto v = testRailway->track(SignalTestRailway::Tracks::V);
            auto w = testRailway->track(SignalTestRailway::Tracks::W);
            auto t3 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(SignalTestRailway::Tracks::Turnout3));

            auto sRA = r->signalGuarding(winston::Track::Connection::A);
            auto sSA = s->signalGuarding(winston::Track::Connection::A);
            auto sSB = s->signalGuarding(winston::Track::Connection::B);
            auto sTA = t->signalGuarding(winston::Track::Connection::A);
            auto sTB = t->signalGuarding(winston::Track::Connection::B);
            auto sUA = u->signalGuarding(winston::Track::Connection::A);
            auto sVA = v->signalGuarding(winston::Track::Connection::A);
            auto sVB = v->signalGuarding(winston::Track::Connection::B);
            auto sWA = w->signalGuarding(winston::Track::Connection::A);
            Assert::IsTrue(sRA.operator bool() == true);
            Assert::IsTrue(sSA.operator bool() == true);
            Assert::IsTrue(sSB.operator bool() == true);
            Assert::IsTrue(sTA.operator bool() == true);
            Assert::IsTrue(sTB.operator bool() == true);
            Assert::IsTrue(sUA.operator bool() == true);
            Assert::IsTrue(sVA.operator bool() == true);
            Assert::IsTrue(sVB.operator bool() == true);
            Assert::IsTrue(sWA.operator bool() == true);

            sRA->aspect(winston::Signal::Aspect::Halt);
            sSA->aspect(winston::Signal::Aspect::Halt);
            sSB->aspect(winston::Signal::Aspect::Halt);
            sTA->aspect(winston::Signal::Aspect::Halt);
            sTB->aspect(winston::Signal::Aspect::Halt);
            sUA->aspect(winston::Signal::Aspect::Halt);
            sVA->aspect(winston::Signal::Aspect::Halt);
            sVB->aspect(winston::Signal::Aspect::Halt);
            sWA->aspect(winston::Signal::Aspect::Halt);

            // S = T3 = T
            signalTower->setSignalsFor(*t3);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sTB->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sUA->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sUA->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVB->shows(winston::Signal::Aspect::Halt));

            signalTower->setSignalOn(*t, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            Assert::IsTrue(sTA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sSA->shows(winston::Signal::Aspect::ExpectGo));

            /*
            signalTower->order(winston::Command::make([t3](const TimePoint &created) -> const winston::State { return t3->finalizeChangeTo(winston::Turnout::Direction::A_C); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Go));*

            signalTower->order(winston::Command::make([t3](const TimePoint &created) -> const winston::State { return t3->finalizeChangeTo(winston::Turnout::Direction::A_B); }));
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));*/
        }
        
        TEST_METHOD(Signals_PreSignal) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto e = testRailway->track(SignalTestRailway::Tracks::E);
            auto f = testRailway->track(SignalTestRailway::Tracks::F);

            auto sVFa = f->signalGuarding(winston::Track::Connection::A);
            auto sHEa = e->signalGuarding(winston::Track::Connection::A);

            sVFa->aspect(winston::Signal::Aspect::Off);
            sHEa->aspect(winston::Signal::Aspect::Off);

            signalTower->setSignalOn(*e, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectGo));

            signalTower->setSignalOn(*e, winston::Track::Connection::A, winston::Signal::Aspect::Halt);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectHalt));
        }

        TEST_METHOD(Signals_ThreeTrackWithPreSignals) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto h = testRailway->track(SignalTestRailway::Tracks::H);
            auto i = testRailway->track(SignalTestRailway::Tracks::I);
            auto j = testRailway->track(SignalTestRailway::Tracks::J);

            auto sHHa = h->signalGuarding(winston::Track::Connection::A);
            auto sHVIa = i->signalGuarding(winston::Track::Connection::A);
            auto sVJa = j->signalGuarding(winston::Track::Connection::A);

            sHHa->aspect(winston::Signal::Aspect::Off);
            sHVIa->aspect(winston::Signal::Aspect::Off);
            sVJa->aspect(winston::Signal::Aspect::Off);

            signalTower->setSignalOn(*h, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
            signalTower->setSignalOn(*h, winston::Track::Connection::A, winston::Signal::Aspect::Halt);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectHalt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
            signalTower->setSignalOn(*i, winston::Track::Connection::A, winston::Signal::Aspect::Halt);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectHalt));
            signalTower->setSignalOn(*h, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectHalt));
            signalTower->setSignalOn(*i, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectGo));
        }
        
        TEST_METHOD(Signals_loopAbort) {
            winston::LocomotiveShed shed;
            auto signalTower = winston::SignalTower::make(shed);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalTower));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto o = testRailway->track(SignalTestRailway::Tracks::O);
            auto p = testRailway->track(SignalTestRailway::Tracks::P);
            auto q = testRailway->track(SignalTestRailway::Tracks::Q);
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(SignalTestRailway::Tracks::Turnout2));

            t2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            auto sHVQa = q->signalGuarding(winston::Track::Connection::A);

            signalTower->setSignalOn(*q, winston::Track::Connection::A, winston::Signal::Aspect::Go);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVQa->shows(winston::Signal::Aspect::Go));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectHalt));

            signalTower->setSignalOn(*q, winston::Track::Connection::A, winston::Signal::Aspect::Halt);
            for (int i = 0; i < 10; ++i)
                signalTower->loop();
            Assert::IsTrue(sHVQa->shows(winston::Signal::Aspect::Halt));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectHalt));
        }
    };
}
