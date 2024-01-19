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
#include "Section.h"
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
			using TurnoutUpdateCallback = std::function<const State(Turnout&  turnout, const Turnout::Direction direction)>;
			TurnoutUpdateCallback turnoutUpdateCallback = [=](winston::Turnout&  turnout, const winston::Turnout::Direction direction) -> const winston::State
			{
				turnout.finalizeChangeTo(direction);
				return winston::State::Finished;
			};
			
			using DoubleSlipUpdateCallback = std::function<const State(DoubleSlipTurnout&  turnout, const DoubleSlipTurnout::Direction direction)>;
			DoubleSlipUpdateCallback doubleSlipUpdateCallback = [=](winston::DoubleSlipTurnout&  turnout, const winston::DoubleSlipTurnout::Direction direction) -> const winston::State
			{
				turnout.finalizeChangeTo(direction);
				return winston::State::Finished;
			};

			using SignalUpdateCallback = std::function<const State(Track&  track, Track::Connection connection, const Signal::Aspects aspect)>;

			DCCDetector::Callback dccDetectorCallback;
		};

		Railway(const Callbacks callbacks);
		virtual ~Railway() = default;

		virtual const Result init() = 0;

	protected:
		const Callbacks callbacks;
	};

	template<typename _TracksClass, typename _RoutesEnum = RoutesNone, typename _SectionsEnum = SectionsSingle>
	class RailwayWithRails : public Railway
	{
	public:
		RailwayWithRails(const Callbacks callbacks) : Railway(callbacks), tracks() { };
		virtual ~RailwayWithRails() = default;

		using Tracks = _TracksClass;

		const Result init()
		{
			auto result = this->initTracks();
			if (result != winston::Result::OK)
			{
				winston::logger.err("Railway::initTracks failed with ", result);
				return result;
			}
			result = this->initRoutes();
			if (result != winston::Result::OK)
			{
				winston::logger.err("Railway::initRoutes failed with ", result);
				return result;
			}
			result = this->initSections();
			if (result != winston::Result::OK)
			{
				winston::logger.err("Railway::initSections failed with ", result);
				return result;
			}

			return winston::Result::OK;
		}

		const Result initTracks()
		{
			for (Tracks track : Tracks::_values())
				this->tracks[track] = this->define(track);
			this->connect();

			return this->validateTracks();
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

		void turnouts(std::function<void(const Tracks track, Turnout& turnout)> callback)
		{
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::Turnout)
					callback(this->trackEnum(i), *(std::static_pointer_cast<Turnout>(this->tracks[i])));
		}

		void doubleSlipTurnouts(std::function<void(const Tracks track, DoubleSlipTurnout& turnout)> callback)
		{
			for (size_t i = 0; i < tracksCount(); ++i)
				if (this->tracks[i]->type() == Track::Type::DoubleSlipTurnout)
					callback(this->trackEnum(i), *(std::static_pointer_cast<DoubleSlipTurnout>(this->tracks[i])));
		}

		inline constexpr Tracks trackEnum(const size_t index) const
		{
			return Tracks::_from_integral_unchecked((unsigned int)index);
		}

		inline Track::Shared track(const std::string name) const
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

		const Tracks trackEnum(const Track::Const& track) const
		{
			auto it = std::find(this->tracks.begin(), this->tracks.end(), track);
			return Tracks::_from_integral_unchecked((unsigned int)std::distance(this->tracks.begin(), it));
		}

		const Tracks trackEnum(const Track& track) const
		{
			auto ptr = track.const_from_this();
			return trackEnum(ptr);
		}
		/*
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
		}*/
	private:
		const Result validateTracks() const
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

		virtual const Result validateFinalTracks()
		{
			return Result::OK;
		};

	public:
		using Routes = _RoutesEnum;
		static constexpr size_t routesCount() noexcept {
			return Routes::_size();
		}
		using RouteArray = std::array<Route::Shared, routesCount()>;

		virtual Route::Shared define(const Routes route)
		{
			return nullptr;
		}

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

		virtual const bool supportRoutes() const
		{
			return false;
		}

		const Result initRoutes()
		{
			if (this->supportRoutes())
			{
				for (Routes route : Routes::_values())
					this->routes[route] = this->define(route);
				// TODO: simulate signals

				return this->validateRoutes();
			}
			else
				return Result::OK;
		}

		using EachRouteCallback = std::function<void(Route::Shared&)>;
		void eachRoute(EachRouteCallback callback)
		{
			for (size_t i = 0; i < routesCount(); ++i)
				callback(this->routes[i]);
		}

		void evaluateConflictingRoutes(EachRouteCallback callback)
		{
			this->eachRoute([callback](Route::Shared& route)
			{
				bool isConflictingSet = false;
				for (auto conflicting : route->getConflictingRoutes())
				{
					isConflictingSet |= conflicting->state() != Route::State::Unset;
					if (isConflictingSet)
						break;
				}

				if (!isConflictingSet)
				{
					route->disable(false);
				}

				if (route->state() == Route::State::Unset)
				{
					route->eachTurnout<true, true>([id = route->id](const winston::Route::Turnout& turnout)
						{
							turnout.turnout()->unlock(id);
						},
						[id = route->id](const winston::Route::DoubleSlipTurnout& turnout)
						{
							turnout.doubleSlipTurnout()->unlock(id);
						}
						);
				}

				callback(route);
			});
		}

	protected:
		const Result validateFinalRoutes()
		{
			for (auto route : this->routes)
			{
				auto result = route->validateSignalPlacemet();
				if (result != Result::OK)
					return result;
			}

			return Result::OK;
		}

	private:
		const std::string enumIntegralToString(const int i)
		{
			if (Routes::_is_valid(i))
				return "unknown";
			
			return Routes::_from_integral_unchecked(i)._to_string();
		}

		Result validateRoutes()
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

			for (size_t i = 0; i < routesCount() - 1; ++i)
			{
				auto current = this->routes[i];
				for (size_t j = i + 1; j < routesCount(); ++j)
				{
					auto other = this->routes[j];

					bool conflicting = false;

					// conflicting routes must not share path elements
					if (!conflicting)
					{
						auto it = current->path.begin();
						while (it != current->path.end() && !conflicting)
						{
							auto jt = other->path.begin();
							while (jt != other->path.end() && !conflicting)
							{
								if ((*it)->track() == (*jt)->track())
									conflicting = true;
								++jt;
							}
							++it;
						}
					}

					// protections on conflicting routes must not contradict each other
					if (!conflicting)
					{
						auto it = current->protections.begin();
						while (it != current->protections.end() && !conflicting)
						{
							auto jt = other->protections.begin();
							while (jt != other->protections.end() && !conflicting)
							{
								if ((*it)->track() == (*jt)->track())
								{
									if ((*it)->type == winston::Route::Track::Type::Turnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);

									}
									else if ((*it)->type == winston::Route::Track::Type::DoubleSlipTurnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);
									}
								}
								++jt;
							}
							++it;
						}
					}

					// protections of a may be part of path of b if they are the same direction
					if (!conflicting)
					{
						auto it = current->protections.begin();
						while (it != current->protections.end() && !conflicting)
						{
							auto jt = other->path.begin();
							while (jt != other->path.end() && !conflicting)
							{
								if ((*it)->track() == (*jt)->track())
								{
									if ((*it)->type == winston::Route::Track::Type::Turnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);

									}
									else if ((*it)->type == winston::Route::Track::Type::DoubleSlipTurnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);
									}
								}
								++jt;
							}
							++it;
						}
					}
					// protections of a may be part of path of b if they are the same direction
					if (!conflicting)
					{
						auto it = current->path.begin();
						while (it != current->path.end() && !conflicting)
						{
							auto jt = other->protections.begin();
							while (jt != other->protections.end() && !conflicting)
							{
								if ((*it)->track() == (*jt)->track())
								{
									if ((*it)->type == winston::Route::Track::Type::Turnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::Turnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);

									}
									else if ((*it)->type == winston::Route::Track::Type::DoubleSlipTurnout)
									{
										auto iTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*it);
										auto jTurnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(*jt);
										conflicting = conflicting || (iTurnout->direction != jTurnout->direction);
									}
								}
								++jt;
							}
							++it;
						}
					}

					if (conflicting)
					{
						current->registerConflictingRoute(other);
						other->registerConflictingRoute(current);
					}
				}
			}

			return passed ? Result::OK : Result::InternalError;
		}

		RouteArray routes;

	public:
		using Sections = _SectionsEnum;

		class Section : public Shared_Ptr<Section>, public winston::Section
		{
		public:
			Section(const Sections id, const Type type, const Trackset trackset) : winston::Section(type, trackset), id{ id }
			{

			};

			const Sections id;
			using Shared_Ptr<Section>::Shared;
			using Shared_Ptr<Section>::make;
		};
		using SectionMap = std::unordered_map<Address, typename Section::Shared>;

		typename Section::Shared section(const Sections section) const
		{
			auto it = this->_sections.find(section);
			if (it != this->_sections.end())
				return it->second;
			return nullptr;
		}

		const SectionMap sections() const
		{
			return this->_sections;
		}
		
		virtual const bool supportSections() const
		{
			return false;
		}

		const Result initSections()
		{
			if (this->supportSections())
			{
				for (Sections section : Sections::_values())
					this->_sections[section] = this->define(section);

				return this->validateSections();
			}
			else
				return Result::OK;
		}

		const Result validateSections()
		{
			std::set<_TracksClass> marked;
			for (const auto& section : this->_sections)
			{
				if(section.second && !section.second->validate([&marked, this](const Track& track) -> const bool {
					_TracksClass trackEnum = this->trackEnum(track);
					if (marked.find(trackEnum) != marked.end())
						return false;
					else
						marked.insert(trackEnum);
					return true;
					}))
					return Result::ValidationFailed;
			}

			return marked.size() == _TracksClass::_size() ? Result::OK : Result::ValidationFailed;
		}

		virtual typename Section::Shared define(const Sections section)
		{
			return nullptr;
		}
		
	protected:
		SectionMap _sections;
	};

}
