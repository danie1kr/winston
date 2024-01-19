#pragma once

#include "HAL.h"
#include "Util.h"
#include <deque>
#include <string>

#ifndef WINSTON_LOG_SIZE
#define WINSTON_LOG_SIZE 256
#endif

namespace winston
{
	enum class LogLevel : unsigned char
	{
		Info,
		Warning,
		Error,
		Fatal
	};

	template<size_t _WINSTON_LOG_SIZE>
	class Log
	{
	public:

		struct Entry {
			using Level = LogLevel;
			const TimePoint timestamp;
			const Level level;
			const std::string text;

			Entry(const TimePoint timestamp, const typename Entry::Level level, std::string text)
				: timestamp(timestamp), level(level), text(std::move(text))
			{
			};

			const std::string levelName() const
			{
#define WINSTON_LOG_LEVEL_ENTRY_CASE(l)	case LogLevel::l: return #l;
				switch (this->level)
				{
					WINSTON_LOG_LEVEL_ENTRY_CASE(Info);
					WINSTON_LOG_LEVEL_ENTRY_CASE(Warning);
					WINSTON_LOG_LEVEL_ENTRY_CASE(Error);
					WINSTON_LOG_LEVEL_ENTRY_CASE(Fatal);
				}
#undef WINSTON_LOG_LEVEL_ENTRY_CASE
				return "???";
			}

			const std::string build() const
			{
				return winston::build(this->timestamp, " - [", this->levelName(), "]: ", this->text);
			}
		};

		using LogEntryCallback = std::function<void(const Entry&)>;

		void log(std::string text, typename Entry::Level level = Entry::Level::Info, const TimePoint timestamp = winston::hal::now())
		{
			if (this->_log.size() > _WINSTON_LOG_SIZE)
				this->_log.pop_front();
			this->_log.emplace_back(timestamp, level, text);
			const Entry& entry = this->_log.back();
			hal::text(entry.build());
			if (this->callback)
				this->callback(entry);
		}

		template <typename ...Params>
		void info(const Params&&... params)
		{
			this->info(winston::build(std::forward<const Params>(params)...));
		}
		template <typename ...Params>
		void info(Params&&... params)
		{
			this->info(winston::build(std::forward<Params>(params)...));
		}
		inline void info(const std::string text)
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

		void setCallback(LogEntryCallback callback)
		{
			this->callback = callback;
		}

	private:
		std::deque<Entry> _log;
		LogEntryCallback callback = nullptr;
	};
	using Logger = Log<WINSTON_LOG_SIZE>;
	extern Logger logger;
}

