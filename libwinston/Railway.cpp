#include "Railway.h"
#include "Log.h"

namespace winston
{
	Railway::Railway(const Callbacks callbacks) : Shared_Ptr<Railway>(), callbacks(callbacks)
	{
	}

	const bool Railway::supportsBlocks() const
	{
		return false;
	}

	const bool Railway::supportsRoutes() const
	{
		return false;
	}

	void Railway::block(const Address address, const Trackset trackset, const Block::Type type)
	{
		if (this->_blocks.find(address) != this->_blocks.end())
			hal::fatal("block address exists already");

		for (auto& track : trackset)
			track->block(address);

		this->_blocks.insert(std::make_pair(address, Block::make(address, trackset, type)));
	}

	Block::Shared Railway::block(Address address) const
	{
		auto it = this->_blocks.find(address);
		if (it != this->_blocks.end())
			return it->second;
		return nullptr;
	}

	const Blockmap Railway::blocks() const
	{
		return this->_blocks;
	}

	void Railway::buildBlocks(const bool simple) { }
}
