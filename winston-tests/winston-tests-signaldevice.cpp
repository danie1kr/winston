#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"
#include "..\winston\TLC5947_SignalDevice.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	template<typename T>
	class Test_SendDevice : public winston::SendDevice<T>, public winston::GPIODigitalPinOutputDevice, public winston::Shared_Ptr<Test_SendDevice<T>>
	{
	public:
		Test_SendDevice()
			: GPIODigitalPinOutputDevice(0) // we don't care
		{

		}

		const winston::Result send(const std::vector<T> data)
		{
			this->_data = data;
			return winston::Result::OK;
		};

		const std::vector<T> data() { return this->_data; };

		void set(const State value) { };  // we don't care

		std::vector<T> _data;

		using winston::Shared_Ptr<Test_SendDevice<T>>::Shared;
		using winston::Shared_Ptr<Test_SendDevice<T>>::make;
	}; 
	
	TEST_CLASS(TLC5947_SignalDevice_Tests)
    {
    public:
        TEST_METHOD(DevInit)
        {
			Test_SendDevice<unsigned char>::Shared sendDevice = Test_SendDevice<unsigned char>::make();
			TLC5947_SignalDevice::Shared signalDevice = TLC5947_SignalDevice::make(24, sendDevice, sendDevice);
        }
		TEST_METHOD(SignalUpdatePort_0_0)
		{
			Test_SendDevice<unsigned char>::Shared sendDevice = Test_SendDevice<unsigned char>::make();
			TLC5947_SignalDevice::Shared signalDevice = TLC5947_SignalDevice::make(24, sendDevice, sendDevice);

			winston::Port port{ 0 };
			winston::Signal::Shared signal = winston::SignalKS::make([](const winston::Signal::Aspects aspect) -> const winston::State { return winston::State::Finished; }, 0, port);
			{
				signal->aspect(winston::Signal::Aspect::Halt);
				signalDevice->update(signal);
				const std::vector<unsigned int> expect_Halt = { 0x00, winston::Signal::Light::maximum >> 8, winston::Signal::Light::maximum & 0xFF};
			
				auto data = sendDevice->data();
				std::vector<unsigned int> data31To33(data.end() - 6, data.end() - 3);
				Assert::IsTrue(expect_Halt == data31To33);
			}
			{
				signal->aspect(winston::Signal::Aspect::Go);
				signalDevice->update(signal);
				const std::vector<unsigned int> expect_Go = { winston::Signal::Light::maximum >> 4, (winston::Signal::Light::maximum & 0xF) << 4, 0x00 };
				auto data = sendDevice->data();
				std::vector<unsigned int> data34To36(data.end() - 3, data.end());
				Assert::IsTrue(expect_Go == data34To36);
			}
		}
    };
}