
#ifdef WINSTON_PLATFORM_ESP32
#define LGFX_USE_V1

#include "../libwinston/external/ArduinoJson-v7.0.1.h"
#include "../libwinston/Log.h"

#include <LovyanGFX.hpp>
#include <driver/i2c.h>
#include <JPEGDEC.h>

#include "Cinema.h"

Cinema::Cinema(SdFat &sd, winston::hal::DisplayUX::Shared display)
	: sd(sd), _display(display), fileJPEG(), jpegBuffer(nullptr), largestJPEGFileSize(0), jpeg(), movies(), currentFrame(0), currentMovie(0)
{
    jpeg.setPixelType(RGB565_BIG_ENDIAN);
    jpeg.setUserPointer(this);
}
#else
#include "Cinema.h"
Cinema::Cinema(winston::hal::DisplayUX::Shared display)
    : _display(display), movies(), currentFrame(0), currentMovie(0)
{

}
#endif


winston::hal::DisplayUX::Shared Cinema::display()
{
    return this->_display;
}

void Cinema::collectMovies()
{
#ifndef WINSTON_PLATFORM_WIN_x64
    std::string path("/movies/movies.json");
    auto jsonFile = this->sd.open(path.c_str());
    if (jsonFile)
    {
        winston::logger.info("Using movies.json");
        size_t jsonFileSize = jsonFile.size();
        unsigned char* jsonBuffer = (unsigned char*)malloc(jsonFileSize + 1);
        memset(jsonBuffer, 0, jsonFileSize + 1);
        jsonFile.read(jsonBuffer, jsonFileSize);

        JsonDocument json;
        deserializeJson(json, jsonBuffer);
        this->largestJPEGFileSize = json["maxFileSize"].as<size_t>();
        auto jsonMovies = json["movies"].as<JsonArray>();
        for (auto movie : jsonMovies)
        {
            std::string path = std::string("/movies/") + movie["name"].as<std::string>();
            auto frames = movie["frameCount"].as<unsigned int>();
            winston::logger.info("Dir: ", path.c_str(), "Frames: ", frames);
            movies.emplace_back(path, frames);
        }
        jsonFile.close();
        winston::logger.info("Maximum size: ", this->largestJPEGFileSize);
        free(jsonBuffer);
    }
    else
    {
        auto moviesDir = this->sd.open("/movies");

        winston::logger.info("Manual frame counting");
        auto file = moviesDir.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                char fileName[64];
                file.getName(fileName, 64);
                std::string path = std::string("/movies/") + std::string(fileName);
                unsigned int frames = movieFrameStart;

                FsFile frameFile;
                while (frameFile = file.openNextFile())
                {
                    if (largestJPEGFileSize < frameFile.size())
                        largestJPEGFileSize = frameFile.size();
                    ++frames;
                }
                winston::logger.info("Dir: ", path.c_str(), " Frames: ", frames, " Maximum size: ", largestJPEGFileSize);
                movies.emplace_back(path, frames);
            }
            file = moviesDir.openNextFile();
        }
    }

    this->jpegBuffer = (uint8_t*)malloc(sizeof(uint8_t) * this->largestJPEGFileSize);
    if (!this->jpegBuffer)
    {
        winston::logger.err("Cinema.cpp: JPEG alloc error");
        while (true);
}
#else
    std::string path("/");
    unsigned int frames = movieFrameStart;
    movies.emplace_back(path, frames);
#endif
}

void Cinema::play()
{
    if (!this->playing())
    {
        currentMovie = std::rand() % movies.size();
        currentFrame = movieFrameStart;
    }

    if (this->playing())
    {
#ifndef WINSTON_PLATFORM_WIN_x64
        auto& movie = this->movies[currentMovie];
        std::string jpegFileName = movie.path + "/";
        if (currentFrame < 1000)
            jpegFileName += "0";
        if (currentFrame < 100)
            jpegFileName += "0";
        if (currentFrame < 10)
            jpegFileName += "0";
        jpegFileName += std::to_string(currentFrame);
        jpegFileName += ".jpg";
        auto f = this->sd.open(jpegFileName.c_str());
        if (f)
        {
            size_t size = f.size();
            if (size < largestJPEGFileSize)
            {
#ifdef SHOW_TIMINGS
                const unsigned long tStart = millis();
#endif
                f.read(jpegBuffer, size);
#ifdef SHOW_TIMINGS
                const unsigned long tRead = millis();
#endif

                jpeg.openRAM((uint8_t*)jpegBuffer, size, [](JPEGDRAW* pDraw)
                    {
                        Cinema* cinema = (Cinema*)pDraw->pUser;
                        cinema->display()->draw(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
                        return 1;
                    });
                jpeg.decode(0, 0, 0);
                jpeg.close();
#ifdef SHOW_TIMINGS
                const unsigned long tDraw = millis();
#endif
                f.close();

#ifdef SHOW_TIMINGS
                const unsigned long lastFrameDuration = millis() - lastFrame;
                std::string t = std::string("Read: ") + std::to_string(tRead - tStart) + std::string("ms");
                lcd.setCursor(0, 0);
                lcd.print(t.c_str());
                t = std::string("Draw: ") + std::to_string(tDraw - tRead) + std::string("ms");
                lcd.setCursor(0, 16);
                lcd.print(t.c_str());
#endif
            }

            ++currentFrame;

            if (currentFrame > movie.frames)
                currentFrame = 0;
        }
        else
            currentFrame = 0;
#endif
    }
}

void Cinema::stop()
{
	this->currentFrame = 0;
}

bool Cinema::playing()
{
	return this->currentFrame != 0;
}