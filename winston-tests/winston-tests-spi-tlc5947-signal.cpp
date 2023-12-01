#include "CppUnitTest.h"

#include "..\libwinston\Winston.h"

#include "..\winston-hal-x64.h"
#include "..\winston\FT232_Device.h"
#include "..\winston\TLC5947_SignalDevice.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace winstontests
{
	TEST_CLASS(TLC5947SPI)
	{
		SignalInterfaceDevice::Shared signalInterfaceDevice;
		TLC5947::Shared signalDevice;

	public:

		TEST_METHOD_INITIALIZE(init)
		{
			this->signalInterfaceDevice = SignalInterfaceDevice::make(3, 5000000);
			auto TLC5947Off = this->signalInterfaceDevice->getOutputPinDevice(4);
			this->signalInterfaceDevice->init();
			this->signalDevice = TLC5947::make(0, 24, this->signalInterfaceDevice, TLC5947Off);
		}

		TEST_METHOD_CLEANUP(cleanup)
		{
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(spiDirect)
			TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(spiDirect)
		{
			// all off
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			// all on
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0xFF);
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			// first on, rest off
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				data[data.size() - 1] = 0xFF;
				data[data.size() - 2] = 0x0F;
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			// on, off, on
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				data[data.size() - 1] = 0xFF;
				data[data.size() - 2] = 0x0F;
				data[data.size() - 3] = 0x00;
				data[data.size() - 4] = 0xFF;
				data[data.size() - 5] = 0x0F;
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			// first off, second half, third full
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				data[data.size() - 1] = 0x00;
				data[data.size() - 2] = 0xF0;
				data[data.size() - 3] = 0x7F;
				data[data.size() - 4] = 0xFF;
				data[data.size() - 5] = 0x0F;
			/*
			l    2   1   0
			    FFF 7FF 000
			[]	5  4  3  2  1 
				0F FF 7F F0 00
			*/
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			// on, 50%, 1%
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				data[data.size() - 1] = 0xFF;
				data[data.size() - 2] = 0x0F;
				data[data.size() - 3] = 0x80;
				data[data.size() - 4] = 0x08;
				data[data.size() - 5] = 0x00;
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}
		}

		BEGIN_TEST_METHOD_ATTRIBUTE(viaTLC)
			TEST_IGNORE()
		END_TEST_METHOD_ATTRIBUTE()
		TEST_METHOD(viaTLC)
		{
			// all off
			{
				std::vector<unsigned char> data((1 * 24 * 12 / 8) / sizeof(unsigned char), 0);
				this->signalInterfaceDevice->send(data);
				winston::hal::delay(200);
			}

			auto port1{ 0 };
			auto signal1 = winston::SignalKS::make(0, winston::Signal::defaultCallback(), 0, port1);

			signal1->aspect(winston::Signal::Aspect::Halt);
			this->signalDevice->update(*signal1);

			signal1->aspect(winston::Signal::Aspect::Go);
			this->signalDevice->update(*signal1);
		}
	};
};
