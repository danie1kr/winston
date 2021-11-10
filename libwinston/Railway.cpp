#include "Railway.h"

namespace winston
{
	Railway::Railway(const Callbacks callbacks) : Shared_Ptr<Railway>(), callbacks(callbacks)
	{
	}

	void Railway::block(const Address address, const Trackset trackset)
	{
		if (this->_blocks.contains(address))
			hal::fatal("block address exists already");

		for (auto& track : trackset)
			track->block(address);

		this->_blocks.insert(std::make_pair(address, Block::make(address, trackset)));
	}

	Block::Shared Railway::block(Address address)
	{
		return this->_blocks[address];
	}

	const Blockmap Railway::blocks() const
	{
		return this->_blocks;
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
