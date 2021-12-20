#pragma once

#include <deque>
#include <string>
#include "span.hpp"
#include "better_enum.hpp"

#ifndef WINSTON_LOG_SIZE
#define WINSTON_LOG_SIZE 64
#endif

namespace winston
{
	BETTER_ENUM(LogLevel, unsigned int,
		Info,
		Warning,
		Error,
		Fatal
	);

	template<size_t _N>
	class Log
	{
	public:

		struct Entry {
			using Level = LogLevel;
			unsigned long long timestamp;
			Level level;
			std::string text;

			Entry( unsigned long long timestamp, typename Entry::Level level, std::string text)
				: timestamp(timestamp), level(level), text(std::move(text))
			{
			};
		};

		void log(std::string text, typename Entry::Level level = Entry::Level::Info, unsigned long long timestamp = winston::hal::now())
		{
			if (this->_log.size() > _N)
				this->_log.pop_front();
			this->_log.emplace_back(timestamp, level, text);
		}

		const std::deque<Entry> entries() const
		{
			return this->_log;
		}

	private:
		std::deque<Entry> _log;
	};
	using Logger = Log<WINSTON_LOG_SIZE>;
	extern Logger logger;
}

