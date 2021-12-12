#pragma once

#include <string>
#include <memory>

#include "Callback.h"

namespace winston
{
	class Mutex
	{
	public:
		Mutex();
		virtual bool lock();
		virtual void unlock();

	private:
		bool locked;
	};

	class NullMutex : public Mutex
	{
	public:
		bool lock();
		void unlock();
	};

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
	std::string build(const unsigned int first);
	std::string build(const int first);
	std::string build(const long first);
	std::string build(const unsigned char first);
	std::string build(const char* first);
	std::string build(const winston::Result first);
	
	template <typename _First, typename... _Args>
		std::string build(const _First first, const _Args&&... args)
	{
		return build(first) + build(args...);
	}

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE STRINGIZE(__LINE__)
#define CaR(code) { winston::Result r = code; if(r != winston::Result::OK) { winston::error(winston::build(__FILE__ "@" LINE ": " STRINGIZE(code) " -> ", winston::build(r))); return r; } }

	//inline const Result checkResultAndComplain(const Result result, std::string message);

	extern Callback::Shared nop;
}
