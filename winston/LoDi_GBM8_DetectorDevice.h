#pragma once

#include <array>
#include "../libwinston/Detector.h"

class LoDi_GBM8_DetectorDevice : public winston::Shared_Ptr<LoDi_GBM8_DetectorDevice>, public winston::DetectorDevice<8>
{
public:
	LoDi_GBM8_DetectorDevice(const std::string name);
	virtual ~LoDi_GBM8_DetectorDevice() = default;
	virtual const winston::Result init(PortSegmentMap ports, winston::Detector::EnterCallback enterCallback);

private:
};

