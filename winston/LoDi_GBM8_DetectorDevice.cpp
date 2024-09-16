#include "LoDi_GBM8_DetectorDevice.h"

LoDi_GBM8_DetectorDevice::LoDi_GBM8_DetectorDevice(const std::string name)
	: winston::Shared_Ptr<LoDi_GBM8_DetectorDevice>(), winston::DetectorDevice<8>(name)
{

}

const winston::Result LoDi_GBM8_DetectorDevice::init(PortSegmentMap ports, winston::Detector::EnterCallback enterCallback)
{
	unsigned int id = 1;
	for (auto& port : this->ports)
	{
		port = winston::Detector::make(winston::build(this->name, " ", id), ports[id-1], enterCallback);
		++id;
	}
	return winston::Result::OK;
}