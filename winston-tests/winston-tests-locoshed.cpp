#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\libwinston\Signal.h"
#include "..\winston\railways.h"
#include "..\winston\LoDi_API.h"
#include "..\libwinston\HAL.h"
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
    TEST_CLASS(LocoShedTests)
    {
        TEST_METHOD_INITIALIZE(resetDelay)
        {
            winston::hal::delayReset();
        }

        class MemoryStorageInterface : public winston::hal::StorageInterface, winston::Shared_Ptr<MemoryStorageInterface>
        {
        public:
            MemoryStorageInterface(const size_t capacity = 0)
                : winston::Shared_Ptr<MemoryStorageInterface>(), winston::hal::StorageInterface(capacity)
            {

            }

            using winston::Shared_Ptr<MemoryStorageInterface>::make;
            using winston::Shared_Ptr<MemoryStorageInterface>::Shared;

            const winston::Result init()
            {
                this->memory.resize(this->capacity, 0);
                return winston::Result::OK;
            }
            const winston::Result read(const size_t address, std::vector<unsigned char>& content, const size_t length = 1)
            {
                return winston::Result::NotImplemented;
            }
            const winston::Result read(const size_t address, std::string& content, const size_t length = 1)
            {
                return winston::Result::NotImplemented;
            };
            const winston::Result read(const size_t address, unsigned char& content)
            {
                content = this->memory[address];
                return winston::Result::OK;
            }
            const winston::Result read(const size_t address, uint32_t& content)
            {
                content = (uint32_t)(((uint32_t)this->memory[address + 0]) << 0)
                    | (uint32_t)(((uint32_t)this->memory[address + 1]) << 8)
                    | (uint32_t)(((uint32_t)this->memory[address + 2]) << 16)
                    | (uint32_t)(((uint32_t)this->memory[address + 3]) << 24);
                return winston::Result::OK;
            }
            const winston::Result read(const size_t address, uint16_t& content)
            {
                content = (uint16_t)(((uint16_t)this->memory[address + 0]) << 0)
                    | (uint16_t)(((uint16_t)this->memory[address + 1]) << 8);
                return winston::Result::OK;
            }
            const winston::Result read(const size_t address, float& content)
            {
                uint32_t f;
                f = (uint32_t)(((uint32_t)this->memory[address + 0]) << 0)
                    | (uint32_t)(((uint32_t)this->memory[address + 1]) << 8)
                    | (uint32_t)(((uint32_t)this->memory[address + 2]) << 16)
                    | (uint32_t)(((uint32_t)this->memory[address + 3]) << 24);

                memcpy_s(&content, sizeof(content), &f, sizeof(f));

                return winston::Result::OK;
            }
            const winston::Result write(const size_t address, const std::vector<unsigned char>& content, const size_t length = 0)
            {
                return winston::Result::NotImplemented;
            }
            const winston::Result write(const size_t address, const std::string& content, const size_t length = 0)
            {
                return winston::Result::NotImplemented;
            }
            const winston::Result write(const size_t address, const uint32_t content)
            {
                this->memory[address + 0] = (uint8_t)(content >> 0);
                this->memory[address + 1] = (uint8_t)(content >> 8);
                this->memory[address + 2] = (uint8_t)(content >> 16);
                this->memory[address + 3] = (uint8_t)(content >> 24);
                return winston::Result::OK;
            }
            const winston::Result write(const size_t address, const uint16_t content)
            {
                this->memory[address + 0] = (uint8_t)(content >> 0);
                this->memory[address + 1] = (uint8_t)(content >> 8);
                return winston::Result::OK;
            }
            const winston::Result write(const size_t address, const float content)
            {
                uint32_t f;
                memcpy_s(&f, sizeof(f), &content, sizeof(content));

                this->memory[address + 0] = (uint8_t)(f >> 0);
                this->memory[address + 1] = (uint8_t)(f >> 8);
                this->memory[address + 2] = (uint8_t)(f >> 16);
                this->memory[address + 3] = (uint8_t)(f >> 24);

                return winston::Result::OK;
            }

            const winston::Result write(const size_t address, const unsigned char content)
            {
                memory[address] = content;
                return winston::Result::OK;
            }
            const winston::Result sync()
            {
                return winston::Result::OK;
            }
        private:
            std::vector<uint8_t> memory;
        };

        winston::LocomotiveShed locoShed;
        static winston::Locomotive::Callbacks locoCallbacks(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed)
        {
            winston::Locomotive::Callbacks callbacks;
            callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) -> const winston::Result
                {
                    return winston::Result::OK;
                };

            callbacks.functions = [=](const winston::Address address, const uint32_t functions) -> const winston::Result
                {
                    return winston::Result::OK;
                };

            callbacks.signalPassed = signalPassed;

            return callbacks;
        }

        void createLocos(winston::Locomotive::Callbacks::SignalPassedCallback signalPassed)
        {
            winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
            winston::ThrottleSpeedMap speedMap{ {0, 0.f}, {100, 1000.f}, {255, 2550.f} };
            locoShed.add(winston::Locomotive::make(locoCallbacks(signalPassed), 3, standardFunctions, winston::Position::nullPosition(), speedMap, "BR 114", 100.f, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting));
            locoShed.add(winston::Locomotive::make(locoCallbacks(signalPassed), 4, standardFunctions, winston::Position::nullPosition(), speedMap, "E 11", 100.f, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting));
        }

    public:
        TEST_METHOD(LocoShedStore)
        {
            auto msi = MemoryStorageInterface::make(16 * 1024);
            msi->init();
            locoShed.init(msi);
            createLocos([](const winston::Locomotive::Const loco, const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
                {
                    return winston::Result::OK;
                });

            auto loco0 = locoShed.shed()[0];
            auto loco1 = locoShed.shed()[1];

            loco0->setSpeedMap({ {0, 0.f}, {10, 10.f}, {164, 93.f} });

            loco0->drive<true>(true, 10);
            Assert::IsTrue(loco0->speed() == 10.f);

            uint8_t count = 0;
            auto result = locoShed.checkHeader(count);
            Assert::IsTrue(result == winston::Result::OK && count == 0);

            size_t address = 0;
            result = locoShed.getLocoMemoryAddress(loco0, address);
            Assert::IsTrue(result == winston::Result::OK && address == 2);
            locoShed.store(loco0);

            result = locoShed.getLocoMemoryAddress(loco0, address);
            Assert::IsTrue(result == winston::Result::OK && address == 2);
            locoShed.store(loco0);

            count = 0;
            result = locoShed.checkHeader(count);
            Assert::IsTrue(result == winston::Result::OK && count == 1);

            result = locoShed.getLocoMemoryAddress(loco1, address);
            Assert::IsTrue(result == winston::Result::OK && address == 2 + WINSTON_STORAGE_LOCOSHED_STRIDE);
            locoShed.store(loco1);
            result = locoShed.checkHeader(count);
            Assert::IsTrue(result == winston::Result::OK && count == 2);

            loco0->setSpeedMap({ {0, 0.f}, {11, 15.f}, {164, 93.f} });
            loco0->drive<true>(true, 11);
            Assert::IsTrue(loco0->speed() == 15.f);

            locoShed.load(loco0, [](auto) { return nullptr; });
            loco0->drive<true>(true, 10);
            Assert::IsTrue(loco0->speed() == 10.f);
        }
    };
}
