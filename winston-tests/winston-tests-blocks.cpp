#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\railways.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(SectionsTest)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }

        std::shared_ptr<RailwayWithSiding> testRailway;

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
            testRailway = RailwayWithSiding::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);
        }

        TEST_METHOD(SectionValidate)
        {
            testRailway = RailwayWithSiding::make(railwayCallbacks());
            testRailway->init();

            std::set<RailwayWithSiding::Tracks> marked;
            auto marker = [&marked, this](const winston::Track& track) -> const bool {
                RailwayWithSiding::Tracks trackEnum = testRailway->trackEnum(track);
                if (marked.find(trackEnum) != marked.end())
                    return false;
                else
                    marked.insert(trackEnum);
                return true;
                };

            {
                // A ==== T1
                auto A = testRailway->track(RailwayWithSiding::Tracks::A);
                auto T1 = testRailway->track(RailwayWithSiding::Tracks::Turnout1);
                auto section = RailwayWithSiding::Section::make(RailwayWithSidingsSections::A, winston::Section::Type::Platform, winston::TrackSet({ A, T1  }));

                marked.clear();
                auto result = section->validate(marker);
                Assert::IsTrue(result);
            }
            {
                // A and B are not connected
                auto A = testRailway->track(RailwayWithSiding::Tracks::A);
                auto B = testRailway->track(RailwayWithSiding::Tracks::B);
                auto section = RailwayWithSiding::Section::make(RailwayWithSidingsSections::A, winston::Section::Type::Platform, winston::TrackSet({ A, B }));

                marked.clear();
                auto result = section->validate(marker);
                Assert::IsFalse(result);
            }
            {
                // C ==== Turnout1 ==== A
                auto C = testRailway->track(RailwayWithSiding::Tracks::C);
                auto Turnout1 = testRailway->track(RailwayWithSiding::Tracks::Turnout1);
                auto A = testRailway->track(RailwayWithSiding::Tracks::A);
                auto section = RailwayWithSiding::Section::make(RailwayWithSidingsSections::A, winston::Section::Type::Platform, winston::TrackSet({ C, Turnout1, A }));

                marked.clear();
                auto result = section->validate(marker);
                Assert::IsTrue(result);
            }
            {
                // C ==== [Turnout1/forgotten] ==== A
                auto C = testRailway->track(RailwayWithSiding::Tracks::C);
                auto Turnout1 = testRailway->track(RailwayWithSiding::Tracks::Turnout1);
                auto A = testRailway->track(RailwayWithSiding::Tracks::A);
                auto section = RailwayWithSiding::Section::make(RailwayWithSidingsSections::A, winston::Section::Type::Platform, winston::TrackSet({ C, A }));

                marked.clear();
                auto result = section->validate(marker);
                Assert::IsFalse(result);
            }
        }

        TEST_METHOD(EntrySetGeneration)
        {
            testRailway = RailwayWithSiding::make(railwayCallbacks());
            Assert::IsTrue(testRailway->init() == winston::Result::OK);

            // C ==== Turnout1 ==== A
            auto C = testRailway->track(RailwayWithSiding::Tracks::C);
            auto Turnout1 = testRailway->track(RailwayWithSiding::Tracks::Turnout1);
            auto A = testRailway->track(RailwayWithSiding::Tracks::A);
            auto section = RailwayWithSiding::Section::make(RailwayWithSidingsSections::A, winston::Section::Type::Platform, winston::TrackSet({ C, Turnout1, A }));

            {
                winston::SectionEntry entry{ C, winston::Track::Connection::B };
                auto result = section->entriesSet.find(entry) == section->entriesSet.end();
                Assert::IsFalse(result);
            }
            {
                winston::SectionEntry entry{ C, winston::Track::Connection::A };
                auto result = section->entriesSet.find(entry) == section->entriesSet.end();
                Assert::IsTrue(result);
            }
        }
    };
}
