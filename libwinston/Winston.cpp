#include "Winston.h"
#include "HAL.h"

namespace winston
{
	void error(std::string error)
	{
		hal::text("error: " + error);
	}
}