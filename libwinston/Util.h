#pragma once

#include <string>
#include <deque>
#include <memory>

#include "WinstonConfig.h"
#include "Callback.h"

namespace winston
{
	extern void error(const std::string &error);

    // https://github.com/friedmud/unique_ptr_cast
	template <typename T_DEST, typename T_SRC, typename T_DELETER>
	std::unique_ptr<T_DEST, T_DELETER>
		dynamic_unique_ptr_cast(std::unique_ptr<T_SRC, T_DELETER>& src)
	{
		if (!src)
			return std::unique_ptr<T_DEST, T_DELETER>(nullptr);

		T_DEST* dest_ptr = dynamic_cast<T_DEST*>(src.get());
		if (!dest_ptr)
			return std::unique_ptr<T_DEST, T_DELETER>(nullptr);

		std::unique_ptr<T_DEST, T_DELETER> dest_temp(dest_ptr, std::move(src.get_deleter()));

		src.release();

		return dest_temp;
	}

	template <typename T_SRC, typename T_DEST>
	std::unique_ptr<T_DEST>
		dynamic_unique_ptr_cast(std::unique_ptr<T_SRC>& src)
	{
		if (!src)
			return std::unique_ptr<T_DEST>(nullptr);

		auto ptr = src.get();
		T_DEST* dest_ptr = dynamic_cast<T_DEST*>(ptr);
		if (!dest_ptr)
			return std::unique_ptr<T_DEST>(nullptr);

		std::unique_ptr<T_DEST> dest_temp(dest_ptr);

		src.release();

		return dest_temp;
	}

	template<typename T> struct is_shared_ptr : std::false_type {};
	template<typename T> struct is_shared_ptr<std::shared_ptr<T>> : std::true_type {};
	
	std::string build();
	std::string build(const std::string first);
	std::string build(const int first);
	std::string build(const unsigned int first);
	std::string build(const long first);
	std::string build(const unsigned long first);
	std::string build(const long long first);
	std::string build(const unsigned long long first);
	std::string build(const short first);
	std::string build(const unsigned short first);
	std::string build(const unsigned char first);
	std::string build(const char* first);
	std::string build(const Result first);
#ifdef WINSTON_HAS_CHRONO
	std::string build(const TimePoint first);
#endif
	template <typename _First, typename... _Args>
		std::string build(const _First first, _Args&&... args)
	{
		return build(first) + build(args...);
	}

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE STRINGIZE(__LINE__)
#define CaR(code) { winston::Result r = code; if(r != winston::Result::OK) { winston::error(winston::build(__FILE__ "@" LINE ": " STRINGIZE(code) " -> ", winston::build(r))); return r; } }

	//inline const Result checkResultAndComplain(const Result result, std::string message);

	unsigned char reverse(unsigned char b);

	extern Callback::Shared nop;

#define WINSTON_JOURNAL_SIZE	32
	class StopwatchJournal
	{
	public:
		struct Event
		{
			Event(StopwatchJournal& swj, const std::string name);
			virtual ~Event();

		private:
			StopwatchJournal& swj;
			const std::string name;
			TimePoint start;
		};

		struct Entry
		{
			Entry(const size_t duration, const std::string name);

			const size_t duration;
			const std::string name;
		};

		struct Statistics
		{
			size_t avg, min, max, stddev;
			std::vector<Entry> top;
			Statistics();
		};

		StopwatchJournal(const std::string name);

		const Statistics stats(const size_t withTop = 0) const;
		const std::string toString(const size_t withTop = 0) const;
		void add(Duration duration, std::string text);

	private:
		const std::string name;
		std::deque<Entry> journal;
	};
}
