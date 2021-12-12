#include "../libwinston/better_enum.hpp"
#include "Util.h"
#include "HAL.h"

namespace winston
{
	Mutex::Mutex()
		: locked(false)
	{

	}

	bool Mutex::lock()
	{
		if (this->locked)
			return false;
		else
		{
			this->locked = true;
			return true;
		}
	}

	void Mutex::unlock()
	{
		this->locked = false;
	}
	bool NullMutex::lock()
	{
		return true;
	}
	
	void NullMutex::unlock()
	{
	}
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

	std::string build(const unsigned int first)
	{
		return std::to_string(first);
	}

	std::string build(const int first)
	{
		return std::to_string(first);
	}

	std::string build(const long first)
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
}
