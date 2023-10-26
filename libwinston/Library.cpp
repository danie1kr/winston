#include "Library.h"
#define _USE_MATH_DEFINES
#include <math.h>
#include "Track.h"

namespace winston
{
	namespace library
	{
		namespace track {
			/*
			G1 = 230mm
			DG1 = 119mm

			R2 Bogen 30 ̊, r = 358 mm
			R3 Bogen 30 ̊, r = 419, 6 mm
			R4 Bogen 30 ̊, r = 481, 2 mm
			R5 Bogen 30 ̊, r = 542, 8 mm
			R6 Bogen 30 ̊, r = 604, 4 mm
			R9 Bogen 15 ̊, r = 826, 4 mm
			R10   Bogen 15 ̊, r = 888 mm
			R20   Bogen 5, 06 ̊, r = 1962 mm*/
			const Length Roco::G1 = 2300;
			const Length Roco::G12 = G1 / 2;
			const Length Roco::G14 = G1 / 4;
			const Length Roco::G4 = G1 * 4;

			const Length Roco::DG1 = 1190;
			const Length Roco::D5 = 50;
			const Length Roco::D8 = 80;
			const Length Roco::D12 = 120;

			const Length Roco::R2 = curve(3580.0, 30.0);
			const Length Roco::R3 = curve(4196.0, 30.0);
			const Length Roco::R4 = curve(4812.0, 30.0);
			const Length Roco::R5 = curve(5428.0, 30.0);
			const Length Roco::R6 = curve(6044.0, 30.0);
			const Length Roco::R9 = curve(8264.0, 15.0);
			const Length Roco::R10 = curve(8880.0, 15.0);
			const Length Roco::R20 = curve(19620.0, 5.0);
			/*
			constexpr Length Roco::W15(const winston::Turnout::Direction direction)
			constexpr Length Roco::W10(const winston::Turnout::Direction direction)
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
			constexpr Length Roco::BW23(const winston::Turnout::Direction direction)
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
			constexpr Length Roco::BW34(const winston::Turnout::Direction direction)
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
			constexpr Length Roco::DKW15(const winston::DoubleSlipTurnout::Direction direction)
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
			}*/
		}
	}

    constexpr Length winston::library::track::curve(const double radius, const double degree)
    {
		return static_cast<Length>(radius * 2.0 * M_PI * degree / 360.0);
    }
}
