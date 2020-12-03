#include "railways.h"
#include <stdexcept>
#include <string>

MiniRailway::MiniRailway(const Callbacks callbacks) : winston::RailwayWithRails<MiniRailwaySections>(callbacks) {};

winston::Section::Shared MiniRailway::define(const Sections section)
{
    switch (section) {
    case Sections::A:
    case Sections::B:
    case Sections::C:
        return winston::Bumper::make();
    case Sections::Turnout1:
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
    }
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

RailwayWithSiding::RailwayWithSiding(const Callbacks callbacks) : winston::RailwayWithRails<RailwayWithSidingsSections>(callbacks) {};
RailwayWithSiding::AddressTranslator::AddressTranslator(RailwayWithSiding::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared RailwayWithSiding::AddressTranslator::turnout(const winston::Address address)
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

const winston::Address RailwayWithSiding::AddressTranslator::address(winston::Section::Shared section)
{
    switch (railway->sectionEnum(section))
    {
    default:
    case Sections::Turnout1: return 0; break;
    case Sections::Turnout2: return 1; break;
    }
    return 0;
}

winston::Locomotive::Shared RailwayWithSiding::AddressTranslator::locomotive(const winston::Address address)
{
    return nullptr;
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
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, section == Sections::Turnout2);
    default:
        winston::hal::fatal(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
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

TimeSaverRailway::TimeSaverRailway(const Callbacks callbacks) : winston::RailwayWithRails<TimeSaverRailwaySections>(callbacks) {};
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
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Sections::Turnout2:
    case Sections::Turnout5:
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
    }
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

Y2020Railway::Y2020Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2020RailwaySections>(callbacks) {};
const std::string Y2020Railway::name()
{
    return std::string("Y2020Railway");
}

Y2020Railway::AddressTranslator::AddressTranslator(Y2020Railway::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared Y2020Railway::AddressTranslator::turnout(const winston::Address address)
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

winston::Locomotive::Shared Y2020Railway::AddressTranslator::locomotive(const winston::Address address)
{
    return nullptr;
}

const winston::Address Y2020Railway::AddressTranslator::address(winston::Section::Shared section)
{
    switch (railway->sectionEnum(section))
    {
    default:
    case Sections::Turnout1: return 0; break;
    case Sections::Turnout2: return 1; break;
    }
    return 0;
}

winston::Section::Shared Y2020Railway::define(const Sections section)
{
    switch (section)
    {
    case Sections::A:
    case Sections::B:
    case Sections::C:
    case Sections::D:
    case Sections::E:
    case Sections::F:
    case Sections::G:
        return winston::Bumper::make();
    case Sections::Turnout1:
    case Sections::Turnout2:
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Sections::Turnout3:
    case Sections::Turnout4:
    case Sections::Turnout5:
    case Sections::Turnout6:
    case Sections::Turnout7:
    case Sections::Turnout8:
    case Sections::Turnout9:
        return winston::Turnout::make([this, section](winston::Section::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Section>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("section ") + std::string(magic_enum::enum_name(section)) + std::string("not in switch"));
    }

}

void Y2020Railway::connect(std::array<winston::Section::Shared, sectionsCount()>& sections)
{
    auto a = this->section(Sections::A);
    auto b = this->section(Sections::B);
    auto c = this->section(Sections::C);
    auto d = this->section(Sections::D);
    auto e = this->section(Sections::E);
    auto f = this->section(Sections::F);
    auto g = this->section(Sections::G);
    auto t1 = this->section(Sections::Turnout1);
    auto t2 = this->section(Sections::Turnout2);
    auto t3 = this->section(Sections::Turnout3);
    auto t4 = this->section(Sections::Turnout4);
    auto t5 = this->section(Sections::Turnout5);
    auto t6 = this->section(Sections::Turnout6);
    auto t7 = this->section(Sections::Turnout7);
    auto t8 = this->section(Sections::Turnout8);
    auto t9 = this->section(Sections::Turnout9);

    a->connect(winston::Section::Connection::A, t2, winston::Section::Connection::B)
        ->connect(winston::Section::Connection::A, t4, winston::Section::Connection::B)
        ->connect(winston::Section::Connection::A, t5, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, t1, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::C, t2, winston::Section::Connection::C);

    t1->connect(winston::Section::Connection::B, t3, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, t4, winston::Section::Connection::C);

    t3->connect(winston::Section::Connection::C, b, winston::Section::Connection::A);

    t5->connect(winston::Section::Connection::C, t6, winston::Section::Connection::C)
        ->connect(winston::Section::Connection::A, t7, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::C, t8, winston::Section::Connection::C)
        ->connect(winston::Section::Connection::A, e, winston::Section::Connection::A);

    t6->connect(winston::Section::Connection::B, c, winston::Section::Connection::A);
    t7->connect(winston::Section::Connection::B, d, winston::Section::Connection::A);

    t8->connect(winston::Section::Connection::B, t9, winston::Section::Connection::A)
        ->connect(winston::Section::Connection::B, g, winston::Section::Connection::A);
    t9->connect(winston::Section::Connection::C, f, winston::Section::Connection::A);
}
