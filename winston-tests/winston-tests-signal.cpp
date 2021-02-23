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
            auto a = testRailway->section(SignalTestRailway::Sections::A);
            auto b = testRailway->section(SignalTestRailway::Sections::B);
            auto c = testRailway->section(SignalTestRailway::Sections::C);
            auto t1 = std::dynamic_pointer_cast<winston::Turnout>(testRailway->section(SignalTestRailway::Sections::Turnout1));

            auto sBA = b->signalGuarding(winston::Section::Connection::A);
            auto sCA = c->signalGuarding(winston::Section::Connection::A);
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
            auto e = testRailway->section(SignalTestRailway::Sections::E);
            auto f = testRailway->section(SignalTestRailway::Sections::F);

            auto sVFa = f->signalGuarding(winston::Section::Connection::A);
            auto sHEa = e->signalGuarding(winston::Section::Connection::A);

            sVFa->aspect(winston::Signal::Aspect::Off);
            sHEa->aspect(winston::Signal::Aspect::Off);

            signalBox->setSignalOn(e, true, winston::Section::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Go));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectGo));

            signalBox->setSignalOn(e, true, winston::Section::Connection::A, winston::Signal::Aspect::Halt, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHEa->shows(winston::Signal::Aspect::Halt));
            Assert::IsTrue(sVFa->shows(winston::Signal::Aspect::ExpectHalt));
        }

        TEST_METHOD(Signals_PreMainSignalNotChanged) {
            winston::NullMutex nullMutex;
            auto signalBox = winston::SignalBox::make(nullMutex);

            testRailway = SignalTestRailway::make(railwayCallbacksWithSignals(signalBox));
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
            auto h = testRailway->section(SignalTestRailway::Sections::H);
            auto i = testRailway->section(SignalTestRailway::Sections::I);
            auto j = testRailway->section(SignalTestRailway::Sections::J);

            auto sHHa = h->signalGuarding(winston::Section::Connection::A);
            auto sHVIa = i->signalGuarding(winston::Section::Connection::A);
            auto sVJa = j->signalGuarding(winston::Section::Connection::A);

            sHHa->aspect(winston::Signal::Aspect::Off);
            sHVIa->aspect(winston::Signal::Aspect::Off);
            sVJa->aspect(winston::Signal::Aspect::Off);

            signalBox->setSignalOn(h, true, winston::Section::Connection::A, winston::Signal::Aspect::Go, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectGo));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
            signalBox->setSignalOn(h, true, winston::Section::Connection::A, winston::Signal::Aspect::Halt, true);
            for (int i = 0; i < 10; ++i)
                signalBox->work();
            Assert::IsTrue(sHVIa->shows(winston::Signal::Aspect::ExpectHalt));
            Assert::IsTrue(sVJa->shows(winston::Signal::Aspect::Off));
        }

        /*
        TEST_METHOD(Signals_loopAbort) {
        }*/
    };
}
