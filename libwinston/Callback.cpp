#include "Callback.h"

namespace winston
{
	Callback::Callback(Function function)
		: Shared_Ptr<Callback>(), function(function), count(0)
	{
	}

	void Callback::execute()
	{
		--count;
		if (this->count == 0)
			this->function();
	}

	Callback::Shared Callback::use()
	{
		++count;
		return this->shared_from_this();
	}
}