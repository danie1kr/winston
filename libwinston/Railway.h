#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <bitset>
#include <memory>
#include <set>
#include <queue>

#include "better_enum.hpp"

#include "WinstonTypes.h"
#include "Util.h"
#include "Track.h"
#include "Block.h"
#include "HAL.h"

namespace winston
{
	class Railway : public Shared_Ptr<Railway>
	{
	public:
		struct Callbacks
		{
			using TurnoutUpdateCallback = std::function<const State(Turnout::Shared turnout, const Turnout::Direction direction)>;
			TurnoutUpdateCallback turnoutUpdateCallback;

			using SignalUpdateCallback = std::function<const State(Track::Shared track, Track::Connection connection, const Signal::Aspects aspect)>;
			SignalUpdateCallback signalUpdateCallback;
		};

		Railway(const Callbacks callbacks);
		virtual ~Railway() = default;

		using SignalFactory = std::function < winston::Signal::Shared (winston::Track::Shared track, winston::Track::Connection connection)>;
		SignalFactory KS(const Length distance = 0, const Port port = Port());

		void block(const Address address, const Trackset trackset);
		Block::Shared block(Address address);
		const Blockmap blocks() const;

	protected:
		const Callbacks callbacks;
		Blockmap _blocks;
	};

	template<typename _TracksClass>
	class RailwayWithRails : public Railway
	{
	public:
		RailwayWithRails(const Callbacks callbacks) : Railway(callbacks), tracks() { };
		virtual ~RailwayWithRails() = default;

		using Tracks = _TracksClass;

		Result init(bool blocks = false)
		{
			for (Tracks track : Tracks::_values())//for (size_t track = 0; track < tracksCount(); ++track)
				this->tracks[track] = define(track);// static_cast<Tracks>(track));
			connect(this->tracks);
			if (!blocks)
			{
				Trackset set;
				for (Tracks track : Tracks::_values())//for (size_t track = 0; track < tracksCount(); ++track)
					set.insert(this->track(track));//static_cast<Tracks>(track)));
				this->block(1, set);
			}
			return this->validate();
		}
		static constexpr size_t tracksCount() noexcept {
			return Tracks::_size();
		}

		virtual winston::Track::Shared define(const Tracks track) = 0;
		virtual void connect(std::array < winston::Track::Shared, tracksCount()>& tracks) = 0;

		template<typename _Track, typename ..._args>
		Track::Shared& add(Tracks track, _args && ...args) {
			return this->tracks[static_cast<size_t>(track)] = std::make_shared<_Track>(std::forward<_args>(args)...);
		}

		const Trackset trackset(std::initializer_list<Tracks> tracks)
		{
			Trackset set;
			for (auto track: tracks)
				set.insert(this->track(track));
			return set;
		}

		/*const std::unordered_map<Tracks, Track::Shared> tracks() const
		{
			std::unordered_map<Tracks, Track::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				map.insert(std::make_pair(trackEnum(i), this->tracks[i]));
			return map;
		}*/

		const std::unordered_map<Tracks, Bumper::Shared> bumper() const
		{
			std::unordered_map<Tracks, Bumper::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				if(this->tracks[i]->type() == Track::Type::Bumper)
					map[this->trackEnum(i)] = std::dynamic_pointer_cast<Bumper>(this->tracks[i]);
			return map;
		}

		const std::unordered_map<Tracks, Rail::Shared> rails() const
		{
			std::unordered_map<Tracks, Rail::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::Rail)
					map[this->trackEnum(i)] = std::dynamic_pointer_cast<Rail>(this->tracks[i]);
			return map;
		}

		void turnouts(std::function<void(const Tracks track, Turnout::Shared turnout)> callback)
		{
//			std::unordered_map<Tracks, Turnout::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::Turnout)
					callback(this->trackEnum(i), std::dynamic_pointer_cast<Turnout>(this->tracks[i]));
					//map[this->trackEnum(i)] = std::dynamic_pointer_cast<Turnout>(this->tracks[i]);
			//return map;
		}

		/*
		bool traverse(const Track::Connection from, Track::Shared& on, Track::Shared& onto) const
		{
			return on ? on->traverse(from, onto) : false;
		}*/

		inline constexpr Tracks trackEnum(size_t index) const
		{
			return Tracks::_from_integral((unsigned int)index);
		}
		/*
		inline constexpr unsigned int trackIndex(Tracks track) const
		{
			return magic_enum::enum_integer(track);
		}
*/
		inline constexpr unsigned int trackIndex(Track::Shared track) const
		{
			auto it = std::find(this->tracks.begin(), this->tracks.end(), track);
			return (unsigned int)std::distance(this->tracks.begin(), it);
		}

		inline Track::Shared& track(Tracks index)
		{
			return this->tracks[static_cast<size_t>(index)];
		}

		inline Track::Shared& track(size_t index)
		{
			return this->tracks[index];
		}

		const Tracks trackEnum(Track::Shared& track) const
		{
			auto it = std::find(this->tracks.begin(), this->tracks.end(), track);
			return Tracks::_from_integral((unsigned int)std::distance(this->tracks.begin(), it)); //this->trackEnum(std::distance(this->tracks.begin(), it));
		}

		inline Tracks track(Bumper::Shared& track)
		{
			auto s = std::dynamic_pointer_cast<Track>(track);
			return this->track(s);
		}
		
		inline Tracks track(Rail::Shared& track)
		{
			auto s = std::dynamic_pointer_cast<Track>(track);
			return this->track(s);
		}

		inline Tracks track(Turnout::Shared& track)
		{
			auto s = std::dynamic_pointer_cast<Track>(track);
			return this->track(s);
		}
	private:
		Result validate()
		{
			bool passed = true;

			for(size_t i = 0; i < tracksCount() && passed; ++i)
			{
				auto current = this->tracks[i];
				if (!current)
					return Result::ValidationFailed;

				passed = current->validate() == Result::OK;
			}

			return passed ? Result::OK : Result::InternalError;
		}

	protected:
		std::array<Track::Shared, tracksCount()> tracks;
	};
}
