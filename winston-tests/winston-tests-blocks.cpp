#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(BlocksTest)
    {
        std::shared_ptr<Y2021Railway> testRailway;

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

        TEST_METHOD(BlockValidate)
        {
            testRailway = Y2021Railway::make(railwayCallbacks());
            testRailway->init();

            std::set<Y2021Railway::Tracks> marked;
            auto marker = [&marked, this](const winston::Track& track) -> const bool {
                Y2021Railway::Tracks trackEnum = testRailway->trackEnum(track);
                if (marked.find(trackEnum) != marked.end())
                    return false;
                else
                    marked.insert(trackEnum);
                return true;
                };

            {
                // Turnout5 ==== PBF1
                auto T5 = testRailway->track(Y2021Railway::Tracks::Turnout5);
                auto PBF1 = testRailway->track(Y2021Railway::Tracks::PBF1);
                auto block = Y2021Railway::Block::make(Y2021RailwayBlocks::PBF1, winston::Block::Type::Platform, winston::Trackset({ T5, PBF1  }));

                marked.clear();
                auto result = block->validate(marker);
                Assert::IsTrue(result);
            }
            {
                // Turnout5 and PBF3 are not connected
                auto T5 = testRailway->track(Y2021Railway::Tracks::Turnout5);
                auto PBF3 = testRailway->track(Y2021Railway::Tracks::PBF3);
                auto block = Y2021Railway::Block::make(Y2021RailwayBlocks::PBF1, winston::Block::Type::Platform, winston::Trackset({ T5, PBF3 }));

                marked.clear();
                auto result = block->validate(marker);
                Assert::IsFalse(result);
            }
            {
                // PBF1 ==== Turnout8 ==== B1
                auto PBF1 = testRailway->track(Y2021Railway::Tracks::PBF1);
                auto Turnout8 = testRailway->track(Y2021Railway::Tracks::Turnout8);
                auto B1 = testRailway->track(Y2021Railway::Tracks::B1);
                auto block = Y2021Railway::Block::make(Y2021RailwayBlocks::PBF1, winston::Block::Type::Platform, winston::Trackset({ PBF1, Turnout8, B1 }));

                marked.clear();
                auto result = block->validate(marker);
                Assert::IsTrue(result);
            }
            {
                // PBF1 ==== [Turnout8/forgotten] ==== B1
                auto PBF1 = testRailway->track(Y2021Railway::Tracks::PBF1);
                auto Turnout8 = testRailway->track(Y2021Railway::Tracks::Turnout8);
                auto B1 = testRailway->track(Y2021Railway::Tracks::B1);
                auto block = Y2021Railway::Block::make(Y2021RailwayBlocks::PBF1, winston::Block::Type::Platform, winston::Trackset({ PBF1, B1 }));

                marked.clear();
                auto result = block->validate(marker);
                Assert::IsFalse(result);
            }
        }

        TEST_METHOD(RailInit)
        {
            testRailway = Y2021Railway::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
        }
    };
}
