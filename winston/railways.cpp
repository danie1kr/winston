#include "railways.h"
#include <stdexcept>
#include <string>

MiniRailway::MiniRailway() : winston::RailwayWithRails<MiniRailwaySections>() {};

winston::Section::Shared MiniRailway::define(const Sections section)
{
    switch (section) {
    case Sections::A:
    case Sections::B:
    case Sections::C:
        return winston::Bumper::make();
    case Sections::Turnout1:
        return winston::Turnout::make([this, section](const winston::Turnout::Direction direction) -> winston::Task::State { return this->turnoutCallback(section, direction); }, false);
    }
    throw std::out_of_range(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
}

void MiniRailway::connect(std::array<winston::Section::Shared, sectionsCount()>& sections)
{
    this->section(Sections::A)->connect(winston::Section::Connection::A, this->section(Sections::Turnout1), winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, this->section(Sections::B), winston::Section::Connection::A);
    this->section(Sections::Turnout1)->connect(winston::Section::Connection::C, this->section(Sections::C), winston::Section::Connection::A);
}

const std::string MiniRailway::name()
{
    return std::string("MiniRailway");
}

RailwayWithSiding::RailwayWithSiding() : winston::RailwayWithRails<RailwayWithSidingsSections>() {};
RailwayWithSiding::AddressTranslator::AddressTranslator(RailwayWithSiding::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared RailwayWithSiding::AddressTranslator::turnout(const unsigned int address)
{
    Sections section;
    switch (address)
    {
    default:
    case 0: section = Sections::Turnout1; break;
    case 1: section = Sections::Turnout2; break;
    }
    return std::dynamic_pointer_cast<winston::Turnout>(railway->section(section));
}

winston::Section::Shared RailwayWithSiding::define(const Sections section)
{
    switch (section)
    {
    case Sections::A:
    case Sections::B:
    case Sections::C:
        return winston::Rail::make();
    case Sections::Turnout1:
    case Sections::Turnout2:
        return winston::Turnout::make([this, section](const winston::Turnout::Direction direction) -> winston::Task::State { return this->turnoutCallback(section, direction); }, section == Sections::Turnout2);
    }
}

void RailwayWithSiding::connect(std::array<winston::Section::Shared, sectionsCount()> & sections)
{
    auto a = this->section(Sections::A);
    auto b = this->section(Sections::B);
    auto c = this->section(Sections::C);
    auto t1 = this->section(Sections::Turnout1);
    auto t2 = this->section(Sections::Turnout2);

    a->connect(winston::Section::Connection::A, t1, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, b, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, t2, winston::Section::Connection::C)
        ->connect(winston::Section::Connection::A, a, winston::Section::Connection::B);
    t1->connect(winston::Section::Connection::C, c, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, t2, winston::Section::Connection::B);
}

const std::string RailwayWithSiding::name()
{
    return std::string("RailwayWithSiding");
}

TimeSaverRailway::TimeSaverRailway() : winston::RailwayWithRails<TimeSaverRailwaySections>() {};
winston::Section::Shared TimeSaverRailway::define(const Sections section)
{
    switch (section)
    {
    case Sections::A:
    case Sections::B:
    case Sections::C:
    case Sections::D:
    case Sections::E:
        return winston::Bumper::make();
    case Sections::Turnout1:
    case Sections::Turnout3:
    case Sections::Turnout4:
        return winston::Turnout::make([this, id = Sections::Turnout1](winston::Turnout::Direction direction)->winston::Task::State { return this->turnoutCallback(id, direction); }, true);
    case Sections::Turnout2:
    case Sections::Turnout5:
        return winston::Turnout::make([this, id = Sections::Turnout1](winston::Turnout::Direction direction)->winston::Task::State { return this->turnoutCallback(id, direction); }, false);
    }
    throw std::out_of_range(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
}

void TimeSaverRailway::connect(std::array<winston::Section::Shared, sectionsCount()>& sections)
{
    // counter-wise orientation
    auto a = this->section(Sections::A);
    auto b = this->section(Sections::B);
    auto c = this->section(Sections::C);
    auto d = this->section(Sections::D);
    auto e = this->section(Sections::E);
    auto t1 = this->section(Sections::Turnout1);
    auto t2 = this->section(Sections::Turnout2);
    auto t3 = this->section(Sections::Turnout3);
    auto t4 = this->section(Sections::Turnout4);
    auto t5 = this->section(Sections::Turnout5);

    a->connect(winston::Section::Connection::A, t1, winston::Section::Connection::B)
        ->connect(winston::Section::Connection::A, b, winston::Section::Connection::A);

    t1->connect(winston::Section::Connection::C, t2, winston::Section::Connection::B)
        ->connect(winston::Section::Connection::A, t3, winston::Section::Connection::C)
        ->connect(winston::Section::Connection::A, c, winston::Section::Connection::A);

    t2->connect(winston::Section::Connection::C, t5, winston::Section::Connection::C)
        ->connect(winston::Section::Connection::A, d, winston::Section::Connection::A);

    t3->connect(winston::Section::Connection::B, t4, winston::Section::Connection::B)
        ->connect(winston::Section::Connection::A, t5, winston::Section::Connection::B);

    t4->connect(winston::Section::Connection::C, e, winston::Section::Connection::A);
}
    
const std::string TimeSaverRailway::name()
{
    return std::string("TimeSaverRailway");
}