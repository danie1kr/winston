#include "Route.h"
#include "Log.h"

namespace winston {
	Route::Track::Track(const winston::Track::Shared track)
		: Shared_Ptr<Route::Track>(), type { Type::Track }, _track{ track }
	{}

	Route::Track::Track(const winston::Track::Shared track, const Type type)
		: Shared_Ptr<Route::Track>(), type{ type }, _track{ track }
	{}

	Track::Shared Route::Track::track() const
	{
		return this->_track;
	}

	const bool operator== (const Route::Track& lhs, const Route::Track& rhs)
	{
		return lhs.type == rhs.type && lhs._track == rhs._track;
	}

	Route::Turnout::Turnout(const winston::Turnout::Shared turnout, const winston::Turnout::Direction direction)
		: Track(turnout, Type::Turnout), Shared_Ptr<Route::Turnout>(), direction{ direction }
	{}

	Turnout::Shared Route::Turnout::turnout() const
	{
		return std::dynamic_pointer_cast<winston::Turnout>(this->_track);
	}

	Route::DoubleSlipTurnout::DoubleSlipTurnout(const winston::DoubleSlipTurnout::Shared turnout, const winston::DoubleSlipTurnout::Direction direction)
		: Track(turnout, Type::DoubleSlipTurnout), Shared_Ptr<Route::DoubleSlipTurnout>(), direction{ direction }
	{}

	DoubleSlipTurnout::Shared Route::DoubleSlipTurnout::doubleSlipTurnout() const
	{
		return std::dynamic_pointer_cast<winston::DoubleSlipTurnout>(this->_track);
	}

	Route::Route(const unsigned int id, const std::string name, const Path path, const Protections protections)
		: Shared_Ptr<Route>(), id{ id }, name{ name }, path{ path }, protections{ protections }, _state{ State::Unset }, _disabled{ false }, turnoutsSet{ 0 }, turnoutsSetRequired{ 0 }, _start{ winston::Track::Connection::DeadEnd }, _end{ winston::Track::Connection::DeadEnd }
	{

	}

	const Result Route::validateSignalPlacemet()
	{
		auto it = path.begin();
		auto firstTrack = (*it)->track();
		auto secondTrack = (*(++it))->track();
		this->_start = firstTrack->whereConnects(secondTrack);
		if (firstTrack->signalGuarding(this->start()) == nullptr)
		{
			logger.warn("Route validation failed: No signal on start of " + this->name);
			return Result::ValidationFailed;
		}

		auto jt = path.rbegin();
		auto lastTrack = (*jt)->track();
		auto secondToLastTrack = (*(++jt))->track();
		this->_end = lastTrack->otherConnection(lastTrack->whereConnects(secondToLastTrack));
		if (lastTrack->signalGuarding(this->end()) == nullptr)
		{
			logger.warn("Route validation failed: No signal on end of " + this->name);
			return Result::ValidationFailed;
		}

		return Result::OK;
	}

	const Result Route::validate()
	{
		const auto & current = this->start();
		winston::Track::Connection connection = winston::Track::Connection::DeadEnd, 
			nextConnection = winston::Track::Connection::DeadEnd;

		for (auto it = path.begin(), jt = it + 1; jt != path.end(); ++it, ++jt)
		{
			const auto& current = *it;
			const auto& next = *jt;
			
			connection = current->track()->whereConnects(next->track());
			if (connection == winston::Track::Connection::DeadEnd)
			{
				logger.err("Route validation failed");
				return Result::ValidationFailed;
			}

			if (nextConnection == winston::Track::Connection::DeadEnd)
			{
				if (current->type == Route::Track::Type::Track)
					nextConnection = current->track()->otherConnection(connection);
				else if (current->type == Route::Track::Type::Turnout)
				{
					auto turnout = std::dynamic_pointer_cast<Turnout>(current);
					const auto& turnoutTrack = turnout->turnout();
					nextConnection = turnoutTrack->otherConnection(connection, turnout->direction);
				}
				else if (current->type == Route::Track::Type::DoubleSlipTurnout)
				{
					auto turnout = std::dynamic_pointer_cast<DoubleSlipTurnout>(current);
					const auto& turnoutTrack = turnout->doubleSlipTurnout();
					nextConnection = turnoutTrack->otherConnection(connection, turnout->direction);
				}
			}
			
			if (current->type == Route::Track::Type::Turnout)
			{
				++this->turnoutsSetRequired;
				auto turnout = std::dynamic_pointer_cast<Turnout>(current);
				auto dir = turnout->direction;
				switch (dir)
				{
				case winston::Turnout::Direction::A_B:
					if (!((connection == winston::Track::Connection::A && nextConnection == winston::Track::Connection::B) ||
						(connection == winston::Track::Connection::B && nextConnection == winston::Track::Connection::A)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				case winston::Turnout::Direction::A_C:
					if (!((connection == winston::Track::Connection::A && nextConnection == winston::Track::Connection::C) ||
						(connection == winston::Track::Connection::C && nextConnection == winston::Track::Connection::A)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				default:
				case winston::Turnout::Direction::Changing:
					logger.err("Route validation failed");
					return Result::ValidationFailed;
				}
			}
			else if (current->type == Route::Track::Type::DoubleSlipTurnout)
			{
				++this->turnoutsSetRequired;
				auto turnout = std::dynamic_pointer_cast<DoubleSlipTurnout>(current);
				auto dir = turnout->direction;
				switch (dir)
				{
				case winston::DoubleSlipTurnout::Direction::A_B:
					if (!((connection == winston::Track::Connection::A && nextConnection == winston::Track::Connection::B) ||
						(connection == winston::Track::Connection::B && nextConnection == winston::Track::Connection::A)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				case winston::DoubleSlipTurnout::Direction::A_C:
					if (!((connection == winston::Track::Connection::A && nextConnection == winston::Track::Connection::C) ||
						(connection == winston::Track::Connection::C && nextConnection == winston::Track::Connection::A)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				case winston::DoubleSlipTurnout::Direction::C_D:
					if (!((connection == winston::Track::Connection::C && nextConnection == winston::Track::Connection::D) ||
						(connection == winston::Track::Connection::D && nextConnection == winston::Track::Connection::C)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				case winston::DoubleSlipTurnout::Direction::B_D:
					if (!((connection == winston::Track::Connection::B && nextConnection == winston::Track::Connection::D) ||
						(connection == winston::Track::Connection::D && nextConnection == winston::Track::Connection::B)))
					{
						logger.err("Route validation failed");
						return Result::ValidationFailed;
					}
					break;
				default:
				case winston::DoubleSlipTurnout::Direction::Changing:
					logger.err("Route validation failed");
					return Result::ValidationFailed;
				}
			}
			nextConnection = next->track()->whereConnects(current->track());
		}

		return Result::OK;
	}

	const bool Route::set() const
	{
		return this->_state != State::Unset;
	}

	const Route::State Route::set(const bool set)
	{
		if(this->disabled())
			this->_state = State::Unset;
		else
		{
			this->_state = set ? State::Setting : State::Unset;
			this->turnoutsSet = 0;
		}
		return this->state();
	}

	const bool Route::disabled() const
	{
		return this->_disabled;
	}

	const bool Route::disable(const bool disable)
	{
		if (this->_state == State::Unset)
		{
			this->_disabled = disable;
			this->_state = State::Unset;
		}
		return this->disabled();
	}

	const Route::State Route::state() const
	{
		return this->_state;
	}

	void Route::registerConflictingRoute(Route::Shared route)
	{
		this->conflictingRoutes.push_back(route);
	}

	const Route::ConflictingRoutes Route::getConflictingRoutes() const
	{
		return this->conflictingRoutes;
	}

	const Track::Connection Route::start() const
	{
		return this->_start;
	}

	const Track::Connection Route::end() const
	{
		return this->_end;
	}

	const Track::Connection Route::start(winston::Track::Shared& track) const
	{
		track = this->path.begin()->get()->track();
		return this->start();
	}
	const Track::Connection Route::end(winston::Track::Shared& track) const
	{
		track = this->path.rbegin()->get()->track();
		return this->end();
	}
}