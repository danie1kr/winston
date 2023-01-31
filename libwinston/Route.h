#pragma once

#include <unordered_map>
#include "WinstonTypes.h"
#include "Track.h"

namespace winston {
	class Route : public Shared_Ptr<Route>
	{
	public:
		enum class State : unsigned int
		{
			Unset,
			Setting,
			Set
		};

		struct Track : public Shared_Ptr<Track>
		{
			enum class Type: unsigned int
			{
				Track,
				Turnout,
				DoubleSlipTurnout
			};
			Track(winston::Track::Shared track);
			virtual ~Track() = default;
			friend const bool operator== (const Track& lhs, const Track& rhs);
			const Type type;

			winston::Track::Shared track() const;

			using Shared_Ptr<Track>::Shared;
			using Shared_Ptr<Track>::make;
		protected:
			Track(winston::Track::Shared track, const Type type);
			winston::Track::Shared _track;
		};

		struct Turnout : public Track, public Shared_Ptr<Turnout>
		{
			Turnout(winston::Turnout::Shared turnout, const winston::Turnout::Direction direction);
			virtual ~Turnout() = default;
			const winston::Turnout::Direction direction;

			winston::Turnout::Shared turnout() const;

			using Shared_Ptr<Turnout>::Shared;
			using Shared_Ptr<Turnout>::make;
		};

		struct DoubleSlipTurnout : public Track, public Shared_Ptr<DoubleSlipTurnout>
		{
			DoubleSlipTurnout(winston::DoubleSlipTurnout::Shared turnout, const winston::DoubleSlipTurnout::Direction direction);
			virtual ~DoubleSlipTurnout() = default;
			const winston::DoubleSlipTurnout::Direction direction;

			winston::DoubleSlipTurnout::Shared doubleSlipTurnout() const;

			using Shared_Ptr<DoubleSlipTurnout>::Shared;
			using Shared_Ptr<DoubleSlipTurnout>::make;
		};

		using Path = std::deque<Track::Shared>;
		using Protections = std::vector<Track::Shared>;
		using ConflictingRoutes = std::vector<Route::Shared>;

		Route(const unsigned int id, const std::string name, const Path path, const Protections protections = {});
		~Route() = default;

		const Result validateSignalPlacemet();
		const Result validate();

		const bool set() const;
		const State set(const bool set);

		const bool disabled() const;
		const bool disable(const bool disable);

		const State reportTurnout(winston::Turnout::Shared turnout, winston::Turnout::Direction direction)
		{
			if (!this->disabled())
			{
				this->eachTurnout<true, true>(
					[=](const Turnout& routeTurnout)
					{
						if (routeTurnout.turnout() == turnout && routeTurnout.direction == direction)
						++this->turnoutsSet;
					},
					[](const DoubleSlipTurnout&) {});

				if (this->state() == State::Setting && this->turnoutsSet == this->turnoutsSetRequired)
					this->_state = State::Set;
			}

			return this->state();
		}

		const State reportTurnout(winston::DoubleSlipTurnout::Shared turnout, winston::DoubleSlipTurnout::Direction direction)
		{
			if (!this->disabled())
			{
				this->eachTurnout<true, true>([](const Turnout&) {},
					[=](const DoubleSlipTurnout& routeTurnout)
					{
						if (routeTurnout.doubleSlipTurnout() == turnout && routeTurnout.direction == direction)
						++this->turnoutsSet;
					});

				if (this->state() == State::Setting && this->turnoutsSet == this->turnoutsSetRequired)
					this->_state = State::Set;
			}

			return this->state();
		}

		using TurnoutCallback = std::function<void(const Turnout &)>;
		using DoubleSlipTurnoutCallback = std::function<void(const DoubleSlipTurnout &)>;
		template<bool _fromPath, bool _fromProtections>
		void eachTurnout(TurnoutCallback turnouts, DoubleSlipTurnoutCallback doubleSlipTurnouts)
		{
			if (_fromPath)
			{
				for (const auto& p : this->path)
				{
					switch (p->type)
					{
					case winston::Route::Track::Type::Turnout:
					{
						auto turnout = std::dynamic_pointer_cast<winston::Route::Turnout>(p);
						turnouts(*turnout);
						break;
					}
					case winston::Route::Track::Type::DoubleSlipTurnout:
					{
						auto turnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(p);
						doubleSlipTurnouts(*turnout);
						break;
					}
					default:
						continue;
					}
				}
			}
			if (_fromProtections)
			{
				for (const auto& p : this->protections)
				{
					switch (p->type)
					{
					case winston::Route::Track::Type::Turnout:
					{
						auto turnout = std::dynamic_pointer_cast<winston::Route::Turnout>(p);
						turnouts(*turnout);
						break;
					}
					case winston::Route::Track::Type::DoubleSlipTurnout:
					{
						auto turnout = std::dynamic_pointer_cast<winston::Route::DoubleSlipTurnout>(p);
						doubleSlipTurnouts(*turnout);
						break;
					}
					default:
						continue;
					}
				}
			}
		}

		const winston::Track::Connection start() const;
		const winston::Track::Connection end() const;
		const winston::Track::Connection start(winston::Track::Shared& track) const;
		const winston::Track::Connection end(winston::Track::Shared& track) const;

		using Shared_Ptr<Route>::Shared;
		using Shared_Ptr<Route>::make;

		const unsigned int id;
		const std::string name;
		const Path path;
		const Protections protections;

		const State state() const;

		void registerConflictingRoute(Route::Shared route);
		const ConflictingRoutes getConflictingRoutes() const;
	private:	
		State _state;
		bool _disabled;
		unsigned int turnoutsSet;
		unsigned int turnoutsSetRequired;
		winston::Track::Connection _start, _end;

		ConflictingRoutes conflictingRoutes;
	};
	using Routemap = std::unordered_map<Address, Route::Shared>;
}

