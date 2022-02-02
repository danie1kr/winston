#include "Railway.h"
#include "Log.h"

namespace winston
{
	Railway::Railway(const Callbacks callbacks) : Shared_Ptr<Railway>(), callbacks(callbacks)
	{
	}

	void Railway::block(const Address address, const Trackset trackset)
	{
		if (this->_blocks.find(address) != this->_blocks.end())
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

	Railway::SignalFactory Railway::KS_dummy(const Length distance, const Port port)
	{
		winston::logger.warn("warn: KS_dummy signal on HW 0#21-23");
		return [distance, this](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
			return winston::SignalKS::make([=](const winston::Signal::Aspects aspect)->const winston::State {
				return this->callbacks.signalUpdateCallback(track, connection, aspect);
				}, distance, Port(0, 21));
		};
	}

	Railway::SignalFactory Railway::H(const Length distance, size_t& device, size_t& port)
	{
		return S<SignalH>(distance, device, port);
		/*
		Port devPort(device, port);
		port += SignalH::lightsCount();
		// TODO: ensure port does not overflow
		return [distance, devPort, this](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
			return SignalH::make([=](const winston::Signal::Aspects aspect)->const winston::State {
				return this->callbacks.signalUpdateCallback(track, connection, aspect);
				}, distance, devPort);
		};*/
	}
}
