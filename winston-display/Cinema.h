#pragma once

#include "../libwinston/HAL.h"

#ifdef WINSTON_PLATFORM_EXP32
#define SPI_DRIVER_SELECT 0
#include <SPI.h>
#include <SdFat.h>
#endif

class Cinema
{
public:
#ifdef WINSTON_PLATFORM_EXP32
	Cinema(SdFat& sd, winston::hal::DisplayUX::Shared display);
#else
	Cinema(winston::hal::DisplayUX::Shared display);
#endif
	~Cinema() = default;

	void collectMovies();
	void play();
	void stop();

	bool playing();
	winston::hal::DisplayUX::Shared display();

private:
	const unsigned int movieFrameStart = 1;
	struct Movie
	{
		const std::string path;
		const unsigned int frames;

		Movie(const std::string path, const unsigned int frames) : path(path), frames(frames) { };
	};
	winston::hal::DisplayUX::Shared _display;
#ifdef WINSTON_PLATFORM_EXP32
	SdFat& sd;

	File fileJPEG;
	uint8_t* jpegBuffer;
	size_t largestJPEGFileSize;

	JPEGDEC jpeg;
#endif

	std::vector<Movie> movies;
	unsigned int currentFrame;
	unsigned int currentMovie;
};
