
#ifdef WINSTON_PLATFORM_EXP32
#define LGFX_USE_V1

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
    Serial.printf("Listing directory: %s\n", "/movies");

    File root = this->sd.open("movies");
    if (!root)
    {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory())
    {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file)
    {
        if (file.isDirectory())
        {
            char fileName[64];
            file.getName(fileName, 64);
            std::string path = std::string("/movies") + std::string(fileName);
            unsigned int frames = movieFrameStart;
            /*  File movieDir = SD.open(path.c_str());
              std::string frame = frameFileName(movieFrameStart);
              Serial.print("Dir:"); Serial.print(path.c_str());
              while(movieDir.exists(frame.c_str()))
              {
                ++frames;
                frame = frameFileName(frames);
                Serial.print(" Frames: ") + Serial.println(frames);
              }
              movies.emplace_back(path, frames);
              */
            File frameFile;
            while (frameFile = file.openNextFile())
            {
                if (largestJPEGFileSize < frameFile.size())
                    largestJPEGFileSize = frameFile.size();
                ++frames;
            }
            Serial.print("Dir: "); Serial.print(path.c_str()); Serial.print(" Frames: "); Serial.print(frames); Serial.print(" Maximum size: "); Serial.println(largestJPEGFileSize);
            movies.emplace_back(path, frames);
        }
        file = root.openNextFile();
    }

    this->jpegBuffer = (uint8_t*)malloc(sizeof(uint8_t) * largestJPEGFileSize);
    if (!this->jpegBuffer)
    {
        //lcd.print("JPEG alloc error");
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
        this->_display->setCursor(0, 0);
        auto& movie = this->movies[currentMovie];
        std::string jpegFileName = movie.path + "/";
        //currentFrame = 666;
        if (currentFrame < 1000)
            jpegFileName += "0";
        if (currentFrame < 100)
            jpegFileName += "0";
        if (currentFrame < 10)
            jpegFileName += "0";
        jpegFileName += std::to_string(currentFrame);
        jpegFileName += ".jpg";
        File f = this->sd.open(jpegFileName.c_str());
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