#pragma once
#include <memory>
#include <functional>

#include "WinstonTypes.h"

namespace winston
{
	class Callback : public Shared_Ptr<Callback>, public std::enable_shared_from_this<Callback>
	{
	public:
		using Function = std::function<void()>;

		Callback(Function function);
		void execute();

	protected:
		Shared use();
	private:
		Function function;
		int count;

		friend class Event;
	};
}

