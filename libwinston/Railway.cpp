#include "Railway.h"

namespace winston
{
	Railway::Railway(const Callbacks callbacks) : Shared_Ptr<Railway>(), callbacks(callbacks)
	{
	}

	Railway::SignalFactory Railway::KS(const Length distance)
	{
		return [distance, this](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
			return winston::SignalKS::make([=](const winston::Signal::Aspects aspect)->const winston::State {
				return this->callbacks.signalUpdateCallback(track, connection, aspect);
				}, distance);
		};
	}
}
