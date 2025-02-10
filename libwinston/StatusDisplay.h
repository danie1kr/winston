#pragma once

#include "..\libwinston\WinstonTypes.h"
#include "..\libwinston\Log.h"

namespace winston
{
	/*
	*   | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 | 20 |
	* 0 |     T   T   C       X   X   X   X   X   m    A         v    v    v    v    v    m    V
	* 1 | W       M   e   s   s   a   g   e
	* 2 | W       M   e   s   s   a   g   e
	* 3 | W       M   e   s   s   a   g   e
	* 4 | W       M   e   s   s   a   g   e
	* 5 | I       M   e   s   s   a   g   e
	* 6 | W       M   e   s   s   a   g   e
	* 7 | E       M   e   s   s   a   g   e
	*/
	template<size_t _columns, size_t _rows>
	class StatusDisplay
	{
	public:
		StatusDisplay()
			: status()
		{
			for (size_t r = 0; r < _rows; ++r)
				for (size_t c = 0; c < _columns; ++c)
					status[r][c] = ' ';
		}

		~StatusDisplay() = default;

		virtual const Result init() = 0;
		virtual const Result digitalStationStatus(const bool shortCircuit, const uint16_t temperature, const uint16_t trackAmp, const uint16_t trackVoltage)
		{
			if (shortCircuit)
			{
				put(0, 0, 20, "Track Short Circuit!");
			}
			else
			{
				const auto temp = temperature > 100 ? 100 : temperature;

				const auto tempString = build(temp, "C");
				const auto trackAmpString = build(trackAmp, "mA");
				const auto trackVoltageString = build(trackVoltage, "mV");

				put(0, 1, 3, tempString);
				put(0, 5, 7, trackAmpString);
				put(0, 13, 7, trackVoltageString);
			}

			return this->update();
		}
		virtual const Result log(typename Logger::Entry entry)
		{
			if (entry.level == Logger::Entry::Level::Info)
				return Result::OK;

			// bubble up
			auto row = 1;
			for (size_t r = row; r < this->rows - 1; ++r)
				status[r] = status[r+1];

			const size_t maxLength = _columns;
			const auto text = entry.buildShort(maxLength);
			put(7, 0, maxLength, text);

			return this->update();
		}
	private:
		virtual const Result update() = 0;
		const Result put(const size_t row, const size_t col, const size_t availableLength, const std::string &text)
		{
			auto c = 0;
			const auto length = text.length();
			for (size_t i = availableLength; i > 0 && row < _rows && c < _columns; --i)
			{
				if (i > length)
					status[row][c++] = ' ';
				else
					status[row][c++] = text[length - i];
			}

			return Result::OK;
		}
	protected:
		const size_t rows = _rows;
		const size_t columns = _columns;
		std::array<std::array<char, _columns>, _rows> status;
	};
};

