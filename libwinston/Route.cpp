#include "Route.h"
#include "Log.h"

namespace winston {
	Route::Track::Track(const winston::Track::Shared track)
		: Shared_Ptr<Route::Track>(), _track{ track }, type { Type::Track }
	{}

	Route::Track::Track(const winston::Track::Shared track, const Type type)
		: Shared_Ptr<Route::Track>(), _track{ track }, type{ type }
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

	Route::Route(const unsigned int id, const std::string name, const winston::Track::Connection signalConnection, const Path path, const Protections protections)
		: Shared_Ptr<Route>(), id{ id }, name{ name }, path{ path }, protections{ protections }, _state{ State::Unset }, turnoutsSet{ 0 }, turnoutsSetRequired{ 0 }, signalConnection{ signalConnection }
	{

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
		this->_state = set ? State::Setting : State::Unset;
		this->turnoutsSet = 0;
		return this->state();
	}

	const Route::State Route::state() const
	{
		return this->_state;
	}

	const Track::Connection Route::start() const
	{
		return this->signalConnection;
	}
}