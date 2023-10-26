#pragma once

#include "WinstonTypes.h"
#include "Track.h"
namespace winston
{
	namespace library
	{
		namespace track
		{
			constexpr Length curve(const double radius, const double degree);
			struct Roco
			{
				static const Length G1, G12, G14, G4;
				static const Length DG1, D5, D8, D12;
				static const Length R2, R3, R4, R5, R6, R9, R10, R20;
				static constexpr Length W15(const winston::Turnout::Direction direction)
				{
					switch (direction)
					{
					case Turnout::Direction::A_B:
						return G1;
					case Turnout::Direction::A_C:
						return R10;
					default:
						return 0;
					}

					return 0;
				}
				static constexpr Length W10(const winston::Turnout::Direction direction)
				{
					switch (direction)
					{
					case Turnout::Direction::A_B:
						return G1 + G12;
					case Turnout::Direction::A_C:
						return 2 * R20;
					default:
						return 0;
					}

					return 0;
				}
				static constexpr Length BW23(const winston::Turnout::Direction direction)
				{
					switch (direction)
					{
					case Turnout::Direction::A_B:
						return G14 + R2 + D8;
					case Turnout::Direction::A_C:
						return R2;
					default:
						return 0;
					}

					return 0;
				}
				static constexpr Length BW34(const winston::Turnout::Direction direction)
				{
					switch (direction)
					{
					case Turnout::Direction::A_B:
						return G14 + R3 + D8;
					case Turnout::Direction::A_C:
						return R3;
					default:
						return 0;
					}

					return 0;
				}
				static constexpr Length DKW15(const winston::DoubleSlipTurnout::Direction direction)
				{
					switch (direction)
					{
					case winston::DoubleSlipTurnout::Direction::A_B:
					case winston::DoubleSlipTurnout::Direction::C_D:
						return G1;
					case winston::DoubleSlipTurnout::Direction::A_C:
					case winston::DoubleSlipTurnout::Direction::B_D:
						return R10 - D12;	// not exactly precise but should be close enough
					default: return 0;
					}
					return 0;
				}
			};
		};

	};

}

