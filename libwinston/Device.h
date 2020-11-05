#pragma once

#include <vector>

namespace winston
{
	using Port = size_t;


	class DeviceBase
	{
	public:
	};

	class DeviceOut : public DeviceBase
	{
	public:
		virtual void write(Port port, std::vector<const unsigned char>& data) = 0;
		virtual void write(Port port, const unsigned char data) = 0;
		virtual void submit();
	};

	class DeviceIn : public DeviceBase
	{
	public:
		virtual void read(Port port, std::vector<const unsigned char>& data) = 0;
		virtual void read(Port port, const unsigned char& data) = 0;
		virtual size_t peek() = 0;
	};

	class Device : public DeviceOut, DeviceIn
	{
	public:
		static const unsigned char PWMHigh = 255;
		static const unsigned char High = 1;
		static const unsigned char Low = 0;

	};
}
