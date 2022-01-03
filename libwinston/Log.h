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

	template<size_t _WINSTON_LOG_SIZE>
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
			if (this->_log.size() > _WINSTON_LOG_SIZE)
				this->_log.pop_front();
			hal::text(text);
			this->_log.emplace_back(timestamp, level, text);
		}

		template <typename ...Params>
		void info(Params&&... params)
		{
			this->info(winston::build(std::forward<Params>(params)...));
		}
		inline void info(std::string text)
		{
			this->log(text, Entry::Level::Info);
		}

		template <typename ...Params>
		void err(Params&&... params)
		{
			this->log(winston::build(std::forward<Params>(params)...), Entry::Level::Error);
		}
		inline void err(std::string text)
		{
			this->log(text, Entry::Level::Error);
		}

		template <typename ...Params>
		void warn(Params&&... params)
		{
			this->log(winston::build(std::forward<Params>(params)...), Entry::Level::Warning);
		}
		inline void warn(std::string text)
		{
			this->log(text, Entry::Level::Warning);
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

