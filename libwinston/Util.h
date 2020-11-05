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

	extern Callback::Shared nop;
}