#include <string>
#include <algorithm>
#include <math.h>

#include "Util.h"
#include "HAL.h"

#ifdef WINSTON_PLATFORM_TEENSY
namespace std
{
	template < typename T > std::string to_string(const T& value)
	{
		// as done by arduino: https://github.com/arduino/ArduinoCore-API/blob/e03b65374c614130aa1b11597e07b3b5089a726d/api/Print.cpp#L240
		char buf[8 * sizeof(long) + 1]; // Assumes 8-bit chars plus zero byte.
		char* str = &buf[sizeof(buf) - 1];

		*str = '\0';

		// we only want base 10
		const unsigned int base = 10;
		T n = value;
		do {
			char c = n % base;
			n /= base;

			*--str = c + '0';
		} while (n);

		return std::string(str);
	}
}
#endif

namespace winston
{
	void error(const std::string& error)
	{
		hal::text(error);
	}

	Callback::Shared nop = Callback::make([]() {});

	std::string build()
	{
		return std::string("");
	}

	std::string build(const std::string first)
	{
		return first;
	}
	std::string build(const int first)
	{
		return std::to_string(first);
	}

	std::string build(const unsigned int first)
	{
		return std::to_string(first);
	}

	std::string build(const short first)
	{
		return std::to_string(first);
	}

	std::string build(const unsigned short first)
	{
		return std::to_string(first);
	}

	std::string build(const long first)
	{
		return std::to_string(first);
	}

	std::string build(const unsigned long first)
	{
		return std::to_string(first);
	}

	std::string build(const long long first)
	{
		return std::to_string(first);
	}

	std::string build(const unsigned long long first)
	{
		return std::to_string(first);
	}

	std::string build(const unsigned char first)
	{
		return std::to_string(first);
	}

	std::string build(const char* first)
	{
		return std::string(first);
	}

#ifdef WINSTON_HAS_CHRONO
	std::string build(const winston::TimePoint first)
	{
		return build(inMilliseconds(first.time_since_epoch()));
	}
#endif

	std::string build(const winston::Result first)
	{
		switch (first)
		{
		case Result::OK: return "OK";
		case Result::SendFailed: return "SendFailed";
		case Result::ValidationFailed: return "ValidationFailed";
		case Result::InternalError: return "InternalError";
		case Result::ExternalHardwareFailed: return "ExternalHardwareFailed";
		default: return "unknown Result code";
		}
	}

	unsigned char reverse(unsigned char b) {
		b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
		b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
		b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
		return b;
	}

	// https://stackoverflow.com/a/10330951
	uint16_t int_sqrt32(uint32_t x)
	{
		uint16_t res = 0;
		uint16_t add = 0x8000;
		int i;
		for (i = 0; i < 16; i++)
		{
			uint16_t temp = res | add;
			uint32_t g2 = temp * temp;
			if (x >= g2)
			{
				res = temp;
			}
			add >>= 1;
		}
		return res;
	}

	StopwatchJournal::Event::Event(StopwatchJournal& swj, const std::string name)
		: swj(swj), name(name), start(hal::now())
	{

	}

	StopwatchJournal::Event::~Event()
	{
		swj.add(hal::now() - this->start, this->name);
	}

	StopwatchJournal::Entry::Entry(const size_t duration, const std::string name)
		: duration(duration), name(name)
	{

	}

	StopwatchJournal::Statistics::Statistics() : avg(0), min(1 << 24), max(0), stddev(0) { }

	const StopwatchJournal::Statistics StopwatchJournal::stats(const size_t withTop) const
	{
		Statistics ret;
		size_t sum = 0;
		for(const auto & entry : this->journal)
		{
			sum += entry.duration;
			ret.min = std::min(ret.min, entry.duration);
			ret.max = std::max(ret.max, entry.duration);
		}
		ret.avg = sum / this->journal.size();

		if (withTop)
		{
			ret.top.reserve(withTop);
			size_t max = ret.max + 1;
			for (size_t i = 0; i < withTop && i < this->journal.size(); ++i)
			{
				size_t newMax = 0;
				Entry const * newEntry = nullptr;
				for (const auto& entry : this->journal)
				{
					if (entry.duration < max && entry.duration > newMax)
					{
						newMax = entry.duration;
						newEntry = &entry;
					}
				}
				if(newEntry)
					ret.top.push_back(*newEntry);
				max = newMax;
			}
		}

		sum = 0;
		for (const auto& entry : this->journal)
		{
			sum += (entry.duration - ret.avg) * (entry.duration - ret.avg);
		}
		ret.stddev = int_sqrt32((unsigned int)(sum / this->journal.size()));

		return ret;
	}

	StopwatchJournal::StopwatchJournal(const std::string name)
		: name(name)
	{

	}

	const std::string StopwatchJournal::toString(const size_t withTop) const
	{
		auto s = this->stats(withTop);
		std::string text = winston::build("Stopwatch [", this->name, "] [N:", this->journal.size(), "]"
			", min:", s.min,
			", avg:", s.avg,
			", max:", s.max,
			", std dev:", s.stddev, "\n");
		for (size_t i = 0; i < s.top.size(); ++i)
		{
			std::string iSpaces(""), durationSpaces("");
			if (i < 100) iSpaces.append(" ");
			if (i < 10) iSpaces.append(" ");

			if (s.top[i].duration < 1000000) durationSpaces.append(" ");
			if (s.top[i].duration < 100000) durationSpaces.append(" ");
			if (s.top[i].duration < 10000) durationSpaces.append(" ");
			if (s.top[i].duration < 1000) durationSpaces.append(" ");
			if (s.top[i].duration < 100) durationSpaces.append(" ");
			if (s.top[i].duration < 10) durationSpaces.append(" ");
			text += winston::build("Stopwatch [", this->name, "] [#", iSpaces, i, "] = ", durationSpaces, s.top[i].duration, "us :", s.top[i].name, "\n");
		}
		return text;
	}

	void StopwatchJournal::add(Duration duration, std::string text)
	{
		if (this->journal.size() > WINSTON_JOURNAL_SIZE)
			this->journal.pop_front();

		this->journal.emplace_back(inMicroseconds(duration), text);
	}
}
