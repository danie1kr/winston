#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(DetectorTests)
    {
        std::shared_ptr<Y2021Railway> railway;

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
        TEST_METHOD(PositionMath)
        {
            railway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(railway->init() == winston::Result::OK);

            // deadend-A = length of bumper
            auto PBF1a = railway->track(Y2021RailwayTracks::PBF1a);
            auto PBF1a_deadEnd = winston::Position(PBF1a, winston::Track::Connection::DeadEnd, 0);
            auto PBF1a_A = winston::Position(PBF1a, winston::Track::Connection::A, 0);
            auto A_to_deadEnd = PBF1a_A.minus(PBF1a_deadEnd);
            Assert::IsTrue(A_to_deadEnd == PBF1a->length());

            // N1_A_50 is 50mm before N1_A
            auto N1 = railway->track(Y2021RailwayTracks::N1);
            auto N1_A = winston::Position(N1, winston::Track::Connection::A, 0);
            auto N1_A_50 = winston::Position(N1, winston::Track::Connection::A, 50);
            Assert::IsTrue(N1_A.minus(N1_A_50) == 50);
            Assert::IsTrue(N1_A_50.minus(N1_A) == -50);
        }
    };
}
