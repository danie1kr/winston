#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\TLC5947_SignalDevice.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	template<typename T, unsigned int bits = 8 * sizeof(T)>
	class Test_SendDevice : public winston::SendDevice<T, bits>, public winston::Shared_Ptr<Test_SendDevice<T, bits>>
	{
	public:
		const winston::Result send(const std::vector<T> data)
		{
			this->_data = data;
			return winston::Result::OK;
		};

		const std::vector<T> data() { return this->_data; };

		std::vector<T> _data;

		using winston::Shared_Ptr<Test_SendDevice<T, bits>>::Shared;
		using winston::Shared_Ptr<Test_SendDevice<T, bits>>::make;
	}; 
	
	TEST_CLASS(TLC5947_SignalDevice_Tests)
    {
    public:
        TEST_METHOD(DevInit)
        {
			Test_SendDevice<unsigned int, 12>::Shared sendDevice = Test_SendDevice<unsigned int, 12>::make();
			TLC5947_SignalDevice::Shared signalDevice = TLC5947_SignalDevice::make(1, 24, sendDevice);
        }
		TEST_METHOD(SignalUpdatePort_0_0)
		{
			Test_SendDevice<unsigned int, 12>::Shared sendDevice = Test_SendDevice<unsigned int, 12>::make();
			TLC5947_SignalDevice::Shared signalDevice = TLC5947_SignalDevice::make(1, 24, sendDevice);

			winston::Port port(0, 0);
			winston::Signal::Shared signal = winston::SignalKS::make([](const winston::Signal::Aspects aspect) -> const winston::State { return winston::State::Finished; }, 0, port);
			
			{
				signal->aspect(winston::Signal::Aspect::Halt);
				signalDevice->update(signal);
				const std::vector<unsigned int> expect_Halt = { 0xFF, 0xF0, 0x00};
			
				auto data = sendDevice->data();
				std::vector<unsigned int> firstThree(data.begin() + 0, data.begin() + 3);
				Assert::IsTrue(expect_Halt == firstThree);
			}
			{
				signal->aspect(winston::Signal::Aspect::Go);
				signalDevice->update(signal);
				const std::vector<unsigned int> expect_Go = { 0x00, 0x0F, 0xFF };
				auto data = sendDevice->data();
				std::vector<unsigned int> firstThree(data.begin() + 0, data.begin() + 3);
				Assert::IsTrue(expect_Go == firstThree);
			}
		}
    };
}