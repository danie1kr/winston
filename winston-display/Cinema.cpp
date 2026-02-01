#include "Cinema.h"

#ifdef WINSTON_PLATFORM_ESP32

#include "../libwinston/external/ArduinoJson-v7.4.2.hpp"
using namespace ArduinoJson;
#include "../libwinston/Log.h"

#include <JPEGDEC.h>

JPEGDEC jpeg;

Cinema::Cinema(SdFat &sd, winston::hal::DisplayUX::Shared display)
	: _display(display), sd(sd), fileMoviePack(), jpegBuffer(nullptr), 
    largestJPEGFileSize(0), movies(), moviePlaying(false), 
    lastFrameTime(0), currentMovie(0), targetMSperFrame(1000 / 20)
{

}
#else
Cinema::Cinema(winston::hal::DisplayUX::Shared display)
    : _display(display), movies(), moviePlaying(false), lastFrameTime(0), currentMovie(0), targetMSperFrame(1000 / 20)
{

}
#endif


winston::hal::DisplayUX::Shared Cinema::display()
{
    return this->_display;
}

void Cinema::init()
{
#ifndef WINSTON_PLATFORM_WIN_x64
    this->collectMovies();
    this->checkAndpackMovies();

    this->jpegBuffer = (uint8_t*)winston::hal::malloc(sizeof(uint8_t) * this->largestJPEGFileSize);
    if (!this->jpegBuffer)
    {
        LOG_ERROR("Cinema.cpp: JPEG alloc error");
        while (true);
    }
#endif
}

void Cinema::collectMovies()
{
#ifndef WINSTON_PLATFORM_WIN_x64
    std::string path("/movies/movies.json");
    auto jsonFile = this->sd.open(path.c_str());
    if (jsonFile)
    {
        LOG_INFO("Using movies.json");
        size_t jsonFileSize = jsonFile.size();
        unsigned char* jsonBuffer = (unsigned char*)winston::hal::malloc(jsonFileSize + 1);
        if (jsonBuffer)
        {
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
                if (frames > 0)
                {
                    LOG_INFO("Dir: ", path.c_str(), "Frames: ", frames);
                    movies.emplace_back(path, frames);
                }
            }
            jsonFile.close();
            LOG_INFO("Maximum size: ", this->largestJPEGFileSize);
            free(jsonBuffer);
        }
    }
    else
    {
        auto moviesDir = this->sd.open("/movies");

        LOG_INFO("Manual frame counting");
        auto file = moviesDir.openNextFile();
        while (file)
        {
            if (file.isDirectory())
            {
                char fileName[64];
                file.getName(fileName, 64);
                //auto fileName = file.name();
                std::string path = std::string("/movies/") + std::string(fileName);
                unsigned int frames = movieFrameStart;

                File frameFile;
                while ((frameFile = file.openNextFile()))
                {
                    if (largestJPEGFileSize < frameFile.size())
                        largestJPEGFileSize = frameFile.size();
                    ++frames;
                }
                LOG_INFO("Dir: ", path.c_str(), " Frames: ", frames, " Maximum size: ", largestJPEGFileSize);
                movies.emplace_back(path, frames);
            }
            file = moviesDir.openNextFile();
        }
    }
    /*
    this->jpegBuffer = (uint8_t*)winston::hal::malloc(sizeof(uint8_t) * this->largestJPEGFileSize);
    if (!this->jpegBuffer)
    {
        LOG_ERROR("Cinema.cpp: JPEG alloc error");
        while (true);
}*/
#else
    std::string path("/");
    unsigned int frames = movieFrameStart;
    movies.emplace_back(path, frames);
#endif
}

#ifndef WINSTON_PLATFORM_WIN_x64
const std::string Cinema::frameToFilename(const unsigned int frame)
{
    // ffmpeg created files with setting %04d.jpg
    std::string name;
    if (frame < 1000)
        name += "0";
    if (frame < 100)
        name += "0";
    if (frame < 10)
        name += "0";
    name += std::to_string(frame);
    name += ".jpg";
    return name;
}

void Cinema::packMovie(Movie & movie, const std::string targetFileName, const size_t chunkSize)
{
    if (sd.exists(targetFileName.c_str()))
    {
        LOG_INFO("Cinema::packMovie: deleting ", targetFileName.c_str());
        sd.remove(targetFileName.c_str());
    }

    auto currentFrame = movieFrameStart;
    File target = sd.open(targetFileName.c_str(), O_RDWR | O_CREAT);
	//auto target = sd.open(targetFileName.c_str(), FILE_WRITE, true);
    //this->display()->setCursor(0, 20);
    LOG_INFO("Cinema::packMovie: working on ", targetFileName.c_str());

    //lcd.setCursor(0, 32);
    LOG_INFO("Cinema::packMovie: Getting sizes");
    uint32_t largestFrameOfMovie = 0;
    for (currentFrame = movieFrameStart; currentFrame < movie.frames; ++currentFrame)
    {
        std::string jpegFileName = movie.path + "/" + frameToFilename(currentFrame);
        File f = sd.open(jpegFileName.c_str());
        if (f)
        {
            uint32_t size = f.size();
            if (size < largestFrameOfMovie)
                largestFrameOfMovie = size;
            f.close();
        }
    }
    LOG_INFO("largest file: %dkb\n", largestFrameOfMovie);
    //Serial.printf("largest file: %dkb\n", largestFrameOfMovie);
    //lcd.setCursor(0, 44);
    //lcd.print("Packing");

    uint32_t bufferSize = chunkSize;
    uint8_t* buffer = (uint8_t*)winston::hal::malloc(bufferSize);

    target.write((uint8_t*) & largestFrameOfMovie, sizeof(largestFrameOfMovie));
    uint32_t overallSize = sizeof(uint32_t) * (movie.frames + 1);
    for (currentFrame = movieFrameStart; currentFrame <= movie.frames; ++currentFrame)
    {
        std::string jpegFileName = movie.path + "/" + frameToFilename(currentFrame);
        File f = sd.open(jpegFileName.c_str());
        if (f)
        {
            uint32_t size = f.size();
            target.write((uint8_t*)&size, sizeof(size));

            uint32_t read = 0;
            uint32_t left = size;
            while (read < size)
            {
                uint32_t toRead = min(left, bufferSize);
                f.read(buffer, toRead);
                target.write(buffer, toRead);
                read += toRead;
                if (toRead > left)
                {
                    LOG_ERROR("Cinema::packMovie: issue with toRead left when packing a movie");
                }
                left -= toRead;
            }
            overallSize += size;
            f.close();
        }
        //lcd.setCursor(0, 56);
        //lcd.print(jpegFileName.c_str());
    }
    free(buffer);
    LOG_INFO("File ", targetFileName.c_str(), " size on disk ", target.size(), " content ", overallSize);
    target.close();
    LOG_INFO("done");
}

void Cinema::checkAndpackMovies()
{
    for (auto& movie : this->movies)
    {
        std::string packedFileName = movie.path + ".pack";
        if (!sd.exists(packedFileName.c_str()))
        {
            this->packMovie(movie, packedFileName, 32 * 1024);
            this->display()->displayLoadingScreen();
        }
    }
}

void Cinema::initPackedMovie(const std::string path)
{
    this->fileMoviePack = sd.open(path.c_str());
    if (!fileMoviePack)
    {
        LOG_ERROR("Cinema::initPackedMovie: cannot open packfile");
        return;
    }

    uint32_t largestFrameOfMovie;
    this->fileMoviePack.read((uint8_t*)&largestFrameOfMovie, sizeof(largestFrameOfMovie));

    if (largestFrameOfMovie > this->largestJPEGFileSize)
    {
        this->largestJPEGFileSize = largestFrameOfMovie;
        if (this->jpegBuffer)
            free(this->jpegBuffer);
        LOG_INFO("Cinema::initPackedMovie: realloc ", largestJPEGFileSize, " for jpeg buffer");

        jpegBuffer = (uint8_t*)winston::hal::malloc(sizeof(uint8_t) * largestJPEGFileSize);
        if (!jpegBuffer)
            LOG_ERROR("Cinema::initPackedMovie: cannot alloc memory for movie frame JPEG :(");
    }
}

const bool Cinema::packPlayNextFrame()
{
    if (this->fileMoviePack.available())
    {
        uint32_t frameSize;
        this->fileMoviePack.read((uint8_t*)&frameSize, sizeof(frameSize));
        this->fileMoviePack.read(jpegBuffer, frameSize);

        jpeg.openRAM((uint8_t*)jpegBuffer, frameSize, [](JPEGDRAW* pDraw)
            {
                Cinema* cinema = (Cinema*)pDraw->pUser;
                cinema->display()->draw(pDraw->x, pDraw->y, pDraw->iWidth, pDraw->iHeight, pDraw->pPixels);
                return 1;
            });

        //jpeg.setPixelType(RGB565_BIG_ENDIAN);
        jpeg.setUserPointer(this);
        jpeg.decode(0, 0, 0);
        jpeg.close();

        return true;
    }
    else
        return false;
}
#endif

const unsigned int Cinema::play()
{
    unsigned int returnDelay = 0;
#ifndef WINSTON_PLATFORM_WIN_x64
    if (!this->playing())
    {
        currentMovie = std::rand() % movies.size();
        this->moviePlaying = true;

        auto& movie = this->movies[currentMovie];
        this->initPackedMovie(movie.path + ".pack");
    }

    if (this->playing())
    {
        this->moviePlaying = packPlayNextFrame();

        const unsigned long lastFrameDuration = millis() - lastFrameTime;
        if (lastFrameDuration < targetMSperFrame)
            returnDelay = targetMSperFrame - lastFrameDuration - 1;
        lastFrameTime = millis();
    }

    if (!this->playing())
    {
        this->fileMoviePack.close();
        return 0;
    }
#endif

    return returnDelay;
}

void Cinema::stop()
{
    this->moviePlaying = false;
}

const bool Cinema::playing()
{
	return this->moviePlaying;
}