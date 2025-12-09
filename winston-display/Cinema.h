#pragma once

#include "../libwinston/WinstonConfig.h"
#include "../libwinston/HAL.h"

#ifdef WINSTON_PLATFORM_ESP32
#include <stdint.h>
#include <SPI.h>
#include <SD.h>
#include <JPEGDEC.h>

extern JPEGDEC jpeg;
#endif

class Cinema
{
public:
#ifdef WINSTON_PLATFORM_ESP32
	Cinema(/*SdFat& sd,*/ winston::hal::DisplayUX::Shared display);
#else
	Cinema(winston::hal::DisplayUX::Shared display);
#endif
	~Cinema() = default;

	void init();
	const unsigned int play();
	void stop();

	const bool playing();
	winston::hal::DisplayUX::Shared display();

private:
	const unsigned int movieFrameStart = 1;
	struct Movie
	{
		const std::string path;
		const unsigned int frames;

		Movie(const std::string path, const unsigned int frames) : path(path), frames(frames) { };
	};

	void collectMovies();
	static const std::string frameToFilename(const unsigned int frame);
	void packMovie(Movie& movie, const std::string targetFileName, const size_t chunkSize);
	void checkAndpackMovies();
	void initPackedMovie(const std::string path);
	const bool packPlayNextFrame();

	winston::hal::DisplayUX::Shared _display;
#ifdef WINSTON_PLATFORM_ESP32
	//SdFat& sd;
	SDFS& sd;

	File fileMoviePack;
	uint8_t* jpegBuffer;
	size_t largestJPEGFileSize;
#endif

	std::vector<Movie> movies;
	bool moviePlaying;
	unsigned int lastFrameTime;
	unsigned int currentMovie;

	const unsigned int targetMSperFrame;
};

