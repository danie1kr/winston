#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(SignalRailwayTest)
    {
        std::shared_ptr<SignalTestRailway> testRailway;

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
        TEST_METHOD(Signals_forTurnouts) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalBox));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto a = testRailway->track(SignalTestRailway::Tracks::A);
            auto b = testRailway->track(SignalTestRailway::Tracks::B);
            auto c = testRailway->track(SignalTestRailway::Tracks::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(SignalTestRailway::Tracks::Turnout1));

            auto sBA = b->signalGuarding(winston::Track::Connection::A);
            auto sCA = c->signalGuarding(winston::Track::Connection::A);
            Assert::IsTrue(sBA.operator bool() == true);
            Assert::IsTrue(sCA.operator bool() == true);

            signalBox->setSignalsFor(t1);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));

            signalBox->order(winston::Command::make([t1](const unsigned long& created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_C); }));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Go));

            signalBox->order(winston::Command::make([t1](const unsigned long& created) -> const winston::State { return t1->finalizeChangeTo(winston::Turnout::Direction::A_B); }));
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sBA->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sCA->shows(winston::Signal::Aspect::Halt));
        }
        
        TEST_METHOD(Signals_PreSignal) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalBox));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto e = testRailway->track(SignalTestRailway::Tracks::E);
            auto f = testRailway->track(SignalTestRailway::Tracks::F);

            auto sVFa = f->signalGuarding(winston::Track::Connection::A);
            auto sHEa = e->signalGuarding(winston::Track::Connection::A);

            sVFa->aspect(winston::Signal::Aspect::Off);
            sHEa->aspect(winston::Signal::Aspect::Off);

            signalBox->setSignalOn(e, true, winston::Track::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectGo));

            signalBox->setSignalOn(e, true, winston::Track::Connection::A, winston::Signal::Aspect::Halt, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectHalt));
        }

        TEST_METHOD(Signals_ThreeTrackWithPreSignals) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalBox));
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

            signalBox->setSignalOn(h, true, winston::Track::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
            signalBox->setSignalOn(h, true, winston::Track::Connection::A, winston::Signal::Aspect::Halt, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectHalt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
            signalBox->setSignalOn(i, true, winston::Track::Connection::A, winston::Signal::Aspect::Halt, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectHalt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectHalt));
            signalBox->setSignalOn(h, true, winston::Track::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectHalt));
            signalBox->setSignalOn(i, true, winston::Track::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::ExpectGo));
        }
        
        TEST_METHOD(Signals_loopAbort) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalBox));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto o = testRailway->track(SignalTestRailway::Tracks::O);
            auto p = testRailway->track(SignalTestRailway::Tracks::P);
            auto q = testRailway->track(SignalTestRailway::Tracks::Q);
            auto t2 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->track(SignalTestRailway::Tracks::Turnout2));

            t2->finalizeChangeTo(winston::Turnout::Direction::A_B);
            auto sHVQa = q->signalGuarding(winston::Track::Connection::A);

            signalBox->setSignalOn(q, true, winston::Track::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVQa->shows(winston::Signal::Aspect::Go));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectHalt));

            signalBox->setSignalOn(q, true, winston::Track::Connection::A, winston::Signal::Aspect::Halt, false);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVQa->shows(winston::Signal::Aspect::Go));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsFalse(sHVQa->shows(winston::Signal::Aspect::ExpectHalt));

        }
    };
}