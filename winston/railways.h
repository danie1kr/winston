#pragma once

#include <array>
#include "../libwinston/Winston.h"

enum class MiniRailwaySections : unsigned int
{
    A,
    Turnout1,
    B,
    C
};

class MiniRailway : public winston::RailwayWithRails<MiniRailwaySections>, winston::Shared_Ptr<MiniRailway>
{
    /*
|====A====Turnout1====B====|
             \========C====|
    A-Turnout1-B
              \C
    */

public:

    MiniRailway();
    virtual ~MiniRailway() = default;

    std::function<winston::Task::State(Sections id, winston::Turnout::Direction direction)> turnoutCallback = [](Sections id, winston::Turnout::Direction aspect) {
        return winston::Task::State::Finished;
    };

    static const std::string name();

    using winston::Shared_Ptr<MiniRailway>::Shared;
    using winston::Shared_Ptr<MiniRailway>::make;

private:
    winston::Section::Shared define(const Sections section);
    void connect(std::array < winston::Section::Shared, sectionsCount()>& sections);
};

/*
    /--A--\
    |     |
    \-2-B-1
      \-C-/

A-Turnout1-B-Turnout2-A
          \C/

*/

enum class RailwayWithSidingsSections : unsigned int
{
    A,
    Turnout1,
    B,
    C,
    Turnout2
};

class RailwayWithSiding : public winston::RailwayWithRails<RailwayWithSidingsSections>, winston::Shared_Ptr<RailwayWithSiding>
{
    std::function<winston::Task::State(Sections id, winston::Turnout::Direction direction)> turnoutCallback = [](Sections id, winston::Turnout::Direction aspect) {
        return winston::Task::State::Finished;
    };
public:
    RailwayWithSiding();
    virtual ~RailwayWithSiding() = default;

    static const std::string name();

    using winston::Shared_Ptr<RailwayWithSiding>::Shared;
    using winston::Shared_Ptr<RailwayWithSiding>::make;
private:
    winston::Section::Shared define(const Sections section);
    void connect(std::array < winston::Section::Shared, sectionsCount()>& sections);

public:
    class AddressTranslator : public winston::DigitalCentralStation::AddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<RailwayWithSiding>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address);
        virtual winston::Locomotive::Shared locomotive(const winston::Address address);
        virtual const winston::Address address(winston::Section::Shared section);

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<RailwayWithSiding>::Shared railway;
    };
};

enum class TimeSaverRailwaySections : unsigned int
{
    A,
    Turnout1,
    B,
    Turnout2,
    Turnout3,
    C,
    Turnout4,
    Turnout5,
    D,
    E
};

class TimeSaverRailway : public winston::RailwayWithRails<TimeSaverRailwaySections>, winston::Shared_Ptr<TimeSaverRailway>
{
    /*
|====A====Turnout1====B====|
             /
          Turnout2===============\
            /                     \
|====C====Turnout3 = Turnout4 = Turnout5====D====|
                       /
             |====E====

    A-Turnout1-B
          |
          Turnout2-Turnout5-D
          |           |
        C-Turnout3-Turnout4
                      |
                      E
    */

public:
    TimeSaverRailway();
    virtual ~TimeSaverRailway() = default;

    std::function<winston::Task::State(Sections id, winston::Turnout::Direction direction)> turnoutCallback = [](Sections id, winston::Turnout::Direction aspect) {
        return winston::Task::State::Finished;
    };

    static const std::string name();

    using winston::Shared_Ptr<TimeSaverRailway>::Shared;
    using winston::Shared_Ptr<TimeSaverRailway>::make;
private:
    winston::Section::Shared define(const Sections section);
    void connect(std::array < winston::Section::Shared, sectionsCount()>& sections);
};

enum class Y2020RailwaySections : unsigned int
{
    Turnout1,
    Turnout2,
    Turnout3,
    Turnout4,
    Turnout5,
    Turnout6,
    Turnout7,
    Turnout8,
    Turnout9,
    A,
    B,
    C,
    D,
    E,
    F,
    G
};

class Y2020Railway : public winston::RailwayWithRails<Y2020RailwaySections>, winston::Shared_Ptr<Y2020Railway>
{
    /*
      //==============Turnout5==================\\
     //                 //                       \\
     ||             Turnout6====C====|           ||
     ||               //                         ||
     ||  |====D====Turnout7                      ||
     ||              //           /=======F====| ||
     || |====E====Turnout8====Turnout9====G====| ||
     ||                                          ||
     ||                       --                 ||
     ||                      B                   ||
     \\                     //                   //
      \\====Turnout1====Turnout3====Turnout4====//
                \\                     //
    |====A====Turnout2==================
    */

public:
    Y2020Railway();
    virtual ~Y2020Railway() = default;

    std::function<winston::Task::State(Sections id, winston::Turnout::Direction direction)> turnoutCallback = [](Sections id, winston::Turnout::Direction aspect) {
        return winston::Task::State::Finished;
    };

    static const std::string name();

    using winston::Shared_Ptr<Y2020Railway>::Shared;
    using winston::Shared_Ptr<Y2020Railway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::AddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2020Railway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address);
        virtual winston::Locomotive::Shared locomotive(const winston::Address address);
        virtual const winston::Address address(winston::Section::Shared section);

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2020Railway>::Shared railway;
    };
private:
    winston::Section::Shared define(const Sections section);
    void connect(std::array < winston::Section::Shared, sectionsCount()>& sections);
};