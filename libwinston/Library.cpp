#include "Library.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace winston
{
    namespace library::track
    {
		/*
		G1 = 230mm
		DG1 = 119mm

		R2 Bogen 30 ̊, r = 358 mm
		R3 Bogen 30 ̊, r = 419, 6 mm
		R4 Bogen 30 ̊, r = 481, 2 mm
		R5 Bogen 30 ̊, r = 542, 8 mm
		R6 Bogen 30 ̊, r = 604, 4 mm
		R9Bogen 15 ̊, r = 826, 4 mm
		R10   Bogen 15 ̊, r = 888 mm
		R20   Bogen 5, 06 ̊, r = 1962 mm*/
		const Length Roco::G1 = 2300;
		const Length Roco::DG1 = 1190;

		const Length Roco::R2 = curve(3580.0, 30.0);
		const Length Roco::R3 = curve(4196.0, 30.0);
		const Length Roco::R4 = curve(4812.0, 30.0);
		const Length Roco::R5 = curve(5428.0, 30.0);
		const Length Roco::R6 = curve(6044.0, 30.0);
		const Length Roco::R9 = curve(8264.0, 15.0);
		const Length Roco::R10 = curve(8880.0, 15.0);
		const Length Roco::R20 = curve(19620.0, 5.06);
    }


    constexpr Length winston::library::track::curve(const double radius, const double degree)
    {
		return static_cast<Length>(radius * 2.0 * M_PI * degree / 360.0);
    }
}
