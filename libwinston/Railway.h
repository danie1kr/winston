#pragma once

#ifdef WINSTON_PLATFORM_TEENSY
//#include "pgmspace.h"
#else
//#define FLASHMEM
#endif

#include <vector>
#include <array>
#include <map>
#include <algorithm>
#include <memory>
#include <tuple>

#include "WinstonTypes.h"
#include "Detector.h"
#include "Util.h"
#include "Track.h"
#include "Block.h"
#include "Route.h"
#include "HAL.h"
#include "Log.h"

namespace winston
{
	class Railway : public Shared_Ptr<Railway>
	{
	public:
		struct Callbacks
		{
			using TurnoutUpdateCallback = std::function<const State(Turnout::Shared turnout, const Turnout::Direction direction)>;
			TurnoutUpdateCallback turnoutUpdateCallback;
			
			using DoubleSlipUpdateCallback = std::function<const State(DoubleSlipTurnout::Shared turnout, const DoubleSlipTurnout::Direction direction)>;
			DoubleSlipUpdateCallback doubleSlipUpdateCallback;

			using SignalUpdateCallback = std::function<const State(Track::Shared track, Track::Connection connection, const Signal::Aspects aspect)>;
			SignalUpdateCallback signalUpdateCallback;

			DCCDetector::Callback dccDetectorCallback;
		};

		Railway(const Callbacks callbacks);
		virtual ~Railway() = default;
		/*
		using SignalFactory = std::function < winston::Signal::Shared (winston::Track::Shared track, winston::Track::Connection connection)>;
		template<class _Signal>
		SignalFactory S(const Length distance, size_t& device, size_t& port)
		{
			Port devPort(device, port);
			port += _Signal::lightsCount();
			// TODO: ensure port does not overflow
			return [distance, devPort, this](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
				return _Signal::make([=](const winston::Signal::Aspects aspect)->const winston::State {
					return this->callbacks.signalUpdateCallback(track, connection, aspect);
					}, distance, devPort);
			};
		}
		SignalFactory KS_dummy(const Length distance = 0, const Port port = Port());
		SignalFactory H(const Length distance, size_t& device, size_t& port);*/

		void block(const Address address, const Trackset trackset, const Block::Type type);
		Block::Shared block(Address address) const;
		const Blockmap blocks() const;
		virtual void buildBlocks(const bool simple = true);

		virtual const bool supportsBlocks() const;
		virtual const bool supportsRoutes() const;

		virtual const Result init() = 0;

	protected:
		const Callbacks callbacks;
		Blockmap _blocks;
	};
	/*
	template<typename _DetectorClass>
	class RailwayWithDetector
	{
	public:
		using Detectors = _DetectorClass;
		virtual void attachDetectors() { };
		using DetectorMap = std::map<typename _DetectorClass, Detector::Shared>;
		const DetectorMap& occupancyDetectors()
		{
			return this->detectors;
		}

		DetectorMap detectors;
	};*/

	template<typename _TracksClass>
	class RailwayWithRails : public Railway
	{
	public:
		RailwayWithRails(const Callbacks callbacks) : Railway(callbacks), tracks() { };
		virtual ~RailwayWithRails() = default;

		using Tracks = _TracksClass;

		const Result initRails()
		{
			for (Tracks track : Tracks::_values())
				this->tracks[track] = this->define(track);
			this->connect();

			return this->validate();
		}

		static constexpr size_t tracksCount() noexcept {
			return Tracks::_size();
		}

		virtual winston::Track::Shared define(const Tracks track) = 0;
		virtual void connect() = 0;

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

		const std::unordered_map<Tracks, Bumper::Shared> bumper() const
		{
			std::unordered_map<Tracks, Bumper::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				if(this->tracks[i]->type() == Track::Type::Bumper)
					map[this->trackEnum(i)] = std::static_pointer_cast<Bumper>(this->tracks[i]);
			return map;
		}

		const std::unordered_map<Tracks, Rail::Shared> rails() const
		{
			std::unordered_map<Tracks, Rail::Shared> map;
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::Rail)
					map[this->trackEnum(i)] = std::static_pointer_cast<Rail>(this->tracks[i]);
			return map;
		}

		void turnouts(std::function<void(const Tracks track, Turnout::Shared turnout)> callback)
		{
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::Turnout)
					callback(this->trackEnum(i), std::static_pointer_cast<Turnout>(this->tracks[i]));
		}

		void doubleSlipTurnouts(std::function<void(const Tracks track, DoubleSlipTurnout::Shared turnout)> callback)
		{
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::DoubleSlipTurnout)
					callback(this->trackEnum(i), std::static_pointer_cast<DoubleSlipTurnout>(this->tracks[i]));
		}

		inline constexpr Tracks trackEnum(size_t index) const
		{
			return Tracks::_from_integral_unchecked((unsigned int)index);
		}

		inline constexpr unsigned int trackIndex(Track::Shared track) const
		{
			auto it = std::find(this->tracks.begin(), this->tracks.end(), track);
			return (unsigned int)std::distance(this->tracks.begin(), it);
		}

		inline Track::Shared track(std::string name)
		{
			auto it = std::find_if(this->tracks.begin(), this->tracks.end(),
				[name](const auto& track) {return name.compare(track->name()) == 0; });
			if (it != this->tracks.end())
				return *it;
			else
				return nullptr;
		}

		inline constexpr Track::Shared& track(Tracks index)
		{
			return this->tracks[static_cast<size_t>(index)];
		}

		inline constexpr Track::Shared& track(size_t index)
		{
			return this->tracks[index];
		}

		const Tracks trackEnum(Track::Shared& track) const
		{
			auto it = std::find(this->tracks.begin(), this->tracks.end(), track);
			return Tracks::_from_integral_unchecked((unsigned int)std::distance(this->tracks.begin(), it)); //this->trackEnum(std::distance(this->tracks.begin(), it));
		}

		inline Tracks track(Bumper::Shared& track)
		{
			auto s = std::static_pointer_cast<Track>(track);
			return this->track(s);
		}
		
		inline Tracks track(Rail::Shared& track)
		{
			auto s = std::static_pointer_cast<Track>(track);
			return this->track(s);
		}

		inline Tracks track(Turnout::Shared& track)
		{
			auto s = std::static_pointer_cast<Track>(track);
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

	template<typename _RoutesEnum>
	class RailwayAddonRoutes
	{
	public:
		using Routes = _RoutesEnum;
		static constexpr size_t routesCount() noexcept {
			return Routes::_size();
		}
		using RouteArray = std::array<Route::Shared, routesCount()>;

		virtual Route::Shared define(const Routes route) = 0;

		Route::Shared route(const int i) const
		{
			if (Routes::_is_valid(i))
				return this->routes[i];
			else
				return nullptr;
		}

		Route::Shared route(const _RoutesEnum id) const
		{
			return this->routes[id];
		}

		const Result initRoutes()
		{
			for (Routes route: Routes::_values())
				this->routes[route] = this->define(route);
			// TODO: simulate signals

			return this->validate();
		}

		const bool supportsRoutes() const
		{
			return true;
		}

		using EachRouteCallback = std::function<void(const Route::Shared&)>;
		void eachRoute(EachRouteCallback callback) const
		{
			for (size_t i = 0; i < routesCount(); ++i)
				callback(this->routes[i]);
		}

	private:
		const std::string enumIntegralToString(const int i)
		{
			try
			{
				Routes r = Routes::_from_integral(i);
				return r._to_string();
			}
			catch (std::exception &e)
			{
				(void)e;
			}
			
			return "unknown";
		}

		Result validate()
		{
			bool passed = true;

			for (size_t i = 0; i < routesCount() && passed; ++i)
			{
				auto current = this->routes[i];
				if (!current)
				{
					logger.warn("Route validation, #", enumIntegralToString((int)i), " is null");
					return Result::ValidationFailed;
				}
				auto result = current->validate();
				if (result != Result::OK)
				{
					logger.err("Route validation, #", enumIntegralToString((int)i), " faild with ", result);
					return Result::ValidationFailed;
				}
				passed = result == Result::OK;
			}

			return passed ? Result::OK : Result::InternalError;
		}

		RouteArray routes;
	};
}
