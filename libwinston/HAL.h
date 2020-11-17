#pragma once

#include <string>
#include <vector>

namespace winston
{
	namespace hal
	{
		extern unsigned long now();
		extern void text(const std::string& text);

		extern void delay(const unsigned int ms);

		extern const unsigned char storageRead(const size_t address);
		extern void storageWrite(const size_t address, const uint8_t data);
		extern bool storageCommit();

		extern void init();
	}
}
