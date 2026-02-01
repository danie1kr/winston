#pragma once
#include "../libwinston/WinstonConfig.h"

#include <Arduino.h>
#include "../libwinston/HAL.h"
#include "../libwinston/Log.h"

#ifdef WINSTON_HAL_USE_STORAGE
//#include <SD.h>
#include <SdFat.h>
extern SdFat SD;
class StorageArduino: public winston::hal::StorageInterface, winston::Shared_Ptr<StorageArduino>
{
public:
	StorageArduino(const std::string filename, const size_t maxSize = 0);

	const winston::Result init(const bool rebuildIfTooSmall = false);
	const winston::Result readVector(const size_t address, std::vector<unsigned char>& content, const size_t length = 1);
	const winston::Result readString(const size_t address, std::string& content, const size_t length = 1);
	const winston::Result read(const size_t address, unsigned char& content);
	const winston::Result writeVector(const size_t address, const std::vector<unsigned char>& content, const size_t length = 0);
	const winston::Result writeString(const size_t address, const std::string& content, const size_t length = 0);
	const winston::Result write(const size_t address, const unsigned char content);
	const winston::Result sync();

	using Shared_Ptr<StorageArduino>::Shared;
	using Shared_Ptr<StorageArduino>::make;
private:
	std::string filename;
#ifdef WINSTON_WITH_SDFAT
	File file;
#endif
};
using Storage = StorageArduino;
#endif
