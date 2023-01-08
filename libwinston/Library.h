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
				static const Length W15(const winston::Turnout::Direction direction);
				static const Length W10(const winston::Turnout::Direction direction);
				static const Length BW23(const winston::Turnout::Direction direction);
				static const Length BW34(const winston::Turnout::Direction direction);
				static const Length DKW15(const winston::DoubleSlipTurnout::Direction direction);
			};
		};

	};

}

