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
}