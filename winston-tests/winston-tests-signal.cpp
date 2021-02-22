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

            auto sHFa = f->signalGuarding(winston::Section::Connection::A);
            auto sVEa = e->signalGuarding(winston::Section::Connection::A);

            sHFa->aspect(winston::Signal::Aspect::Off);
            sVEa->aspect(winston::Signal::Aspect::Off);

            //signalBox->
        }

        /*
        TEST_METHOD(Signals_PreMainSignalNotChanged) {
        }


        TEST_METHOD(Signals_loopAbort) {
        }*/
    };
}
