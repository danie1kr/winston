#ifndef WINSTON_UTIL_H
#define WINSTON_UTIL_H
#pragma once

#include <string>
#include <deque>
#include <memory>
#include <functional>
#include <algorithm>
#include <random>
#include <iterator>

#include "WinstonTypes.h"
#include "WinstonConfig.h"

#define WHILE_SAFE_Ex(__wsex_iterations, __wsex_condition, __wsex_code) \
{ const size_t __wsex_line = __LINE__; \
	size_t __wsex_it = 0; \
	while (__wsex_it < __wsex_iterations) \
	{ \
		if (__wsex_condition) { \
			__wsex_code; \
		} else break; \
		++__wsex_it; \
	} \
	if (__wsex_it >= __wsex_iterations) \
	{ \
		winston::logger.err("there seems to be an infinite loop in: ", __FILE__, " at #", __wsex_line);  \
	} \
}

#define WHILE_SAFE(condition, code) { WHILE_SAFE_Ex(1000, condition, code); }

namespace winston
{
	extern std::random_device randomDevice;
	extern std::default_random_engine randomEngine;

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

	const std::string hex(unsigned int n);
	
	const std::string build();
	const std::string build(const std::string first);
	const std::string build(const int first);
	const std::string build(const unsigned int first);
	const std::string build(const long first);
	const std::string build(const unsigned long first);
	const std::string build(const long long first);
	const std::string build(const unsigned long long first);
	const std::string build(const short first);
	const std::string build(const unsigned short first);
	const std::string build(const unsigned char first);
	const std::string build(const char* first);
	const std::string build(const float first);
	const std::string build(const Result first);
#ifdef WINSTON_HAS_CHRONO
	const std::string build(const TimePoint first);
#endif
	template <typename _First, typename... _Args>
		std::string build(const _First first, _Args&&... args)
	{
		return build(first) + build(args...);
	}

	template<typename T>
	const T lerp(const T lower, const T upper, const float frac)
	{
		if (frac <= 0.0f)
			return lower;
		else if (frac >= 1.0f)
			return upper;
		//return (T)(frac * upper + (1.0f - frac) * lower);
		return (T)(lower + frac * (upper-lower));
	}

	template<typename T>
	const T clamp(const T lower, const T upper, const T x)
	{
		return std::min(std::max(x, lower), upper);
	}

	template<typename T>
	const T filter(const T& container, std::function<const bool(const typename T::value_type& data)> condition)
	{
		T result;
		std::copy_if(container.begin(), container.end(), std::back_inserter(result), condition);
		return result;
	}

	template<typename T>
	T random(T& container, const size_t n)
	{
		T result;
		std::shuffle(container.begin(), container.end(), randomEngine);
		result.insert(result.begin(), container.begin(), container.begin() + std::min(n, container.size()));
		return result;
	}

	template<typename T>
	const typename T::value_type& random(const T& container)
	{
		auto start = container.begin();
		if (container.size() <= 1)
			return *start;
		std::uniform_int_distribution<size_t> dis(0, (size_t)(std::distance(container.begin(), container.end()) - 1));
		
		std::advance(start, dis(randomEngine));
		return *start;
	}

	const Result memcpy_s(void* dest, const size_t destSize, const void* src, const size_t srcSize);

	/*
	// from https://gist.github.com/cbsmith/5538174
	template <typename RandomGenerator = std::default_random_engine>
	struct random_selector
	{
		//On most platforms, you probably want to use std::random_device("/dev/urandom")()
		random_selector(RandomGenerator g = RandomGenerator(std::random_device()()))
			: gen(g) {}

		template <typename Iter>
		Iter select(Iter start, Iter end) {
			std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
			std::advance(start, dis(gen));
			return start;
		}

		//convenience function
		template <typename Iter>
		Iter operator()(Iter start, Iter end) {
			return select(start, end);
		}

		//convenience function that works on anything with a sensible begin() and end(), and returns with a ref to the value type
		template <typename Container>
		auto operator()(const Container& c) -> decltype(*begin(c))& {
			return *select(begin(c), end(c));
		}

	private:
		RandomGenerator gen;
	};*/

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE STRINGIZE(__LINE__)
#define CaR(code) { winston::Result r = code; if(r != winston::Result::OK) { winston::error(winston::build(__FILE__ "@" LINE ": " STRINGIZE(code) " -> ", winston::build(r))); return r; } }
#define CheckAndReport(code) { winston::Result r = code; if(r != winston::Result::OK) { winston::error(winston::build(__FILE__ "@" LINE ": " STRINGIZE(code) " -> ", winston::build(r))); } }

	unsigned char reverse(unsigned char b);

	//extern Callback::Shared nop;

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
#endif