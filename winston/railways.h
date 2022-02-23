#pragma once

#ifdef WINSTON_PLATFORM_TEENSY
//#include "pgmspace.h"
#else
//#define FLASHMEM
#endif

#include <array>
#include "../libwinston/better_enum.hpp"
#include "../libwinston/Winston.h"

#ifndef WINSTON_PLATFORM_TEENSY
//enum class MiniRailwayTracks : unsigned int
BETTER_ENUM(MiniRailwayTracks, unsigned int, //{
    A,
    Turnout1,
    B,
    C
);// };

class MiniRailway : public winston::RailwayWithRails<MiniRailwayTracks>, winston::Shared_Ptr<MiniRailway>
{
    /*
|====A====Turnout1====B====|
             \========C====|
    A-Turnout1-B
              \C
    */

public:

    MiniRailway(const Callbacks callbacks);
    virtual ~MiniRailway() = default;

    static const std::string name();

    using winston::Shared_Ptr<MiniRailway>::Shared;
    using winston::Shared_Ptr<MiniRailway>::make;

private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#endif

#ifndef WINSTON_PLATFORM_TEENSY
//enum class SignalTestRailwayTracks : unsigned int
BETTER_ENUM(SignalTestRailwayTracks, unsigned int, //{
    A,
    Turnout1,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Turnout2,
    Q,
    R,
    S,
    Turnout3,
    T,
    U,
    V,
    W,
    L0, L1, L2, L3, L4, L5, L6, L7, L8
);

class SignalTestRailway : public winston::RailwayWithRails<SignalTestRailwayTracks>, winston::Shared_Ptr<SignalTestRailway>
{
    /*
    * |====A====T1====HBa=B====|
    *            \====HCa=C====|
    * 
    * D|      Da-Ea   <  Eb-Fa < F| 
    * |====D====HEa=E====VFa=F====|
    * 
    * G|      Ga-Ha        Hb-Ia    Ib-Ja  J|
    * |====G====HHa=H====HVIa=I====VJa=J====|
    * 
    * K|      Ka-La        Lb-Ma    Mb-Na   N|
    * |====K====KLa=L====KMa=M====KNa=N====|
    * 
    *  /====O====\
      ||         /
       \====P====T2====KVQa=Q====|
    * 
    * 
    * |====R=KRa=KSb=S=KSa=T3=KTb=T=KTa=KUa=U====|
    *                       \=KVb=V=KVa=KWa=W====|
    * 
    * |====L0====KL0a=KL1b====L1====L2====L3====L4=KL4a====L5====L6====L7====KL7a=KL8a====L8====|
    */

public:

    SignalTestRailway(const Callbacks callbacks);
    virtual ~SignalTestRailway() = default;

    static const std::string name();

    using winston::Shared_Ptr<SignalTestRailway>::Shared;
    using winston::Shared_Ptr<SignalTestRailway>::make;

private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#endif

#ifndef WINSTON_PLATFORM_TEENSY
/*
    /--A--\
    |     |
    \-2-B-1
      \-C-/

A-Turnout1-B-Turnout2-A
          \C/

*/

BETTER_ENUM(RailwayWithSidingsTracks, unsigned int, //{
    A,
    Turnout1,
    B,
    C,
    Turnout2
);

class RailwayWithSiding : public winston::RailwayWithRails<RailwayWithSidingsTracks>, winston::Shared_Ptr<RailwayWithSiding>
{
public:
    RailwayWithSiding(const Callbacks callbacks);
    virtual ~RailwayWithSiding() = default;

    static const std::string name();

    using winston::Shared_Ptr<RailwayWithSiding>::Shared;
    using winston::Shared_Ptr<RailwayWithSiding>::make;
private:
    winston::Track::Shared define(const Tracks track);
    void connect();

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<RailwayWithSiding>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<RailwayWithSiding>::Shared railway;
    };
};
#endif

#ifndef WINSTON_PLATFORM_TEENSY
BETTER_ENUM(TimeSaverRailwayTracks, unsigned int, //{
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
);

class TimeSaverRailway : public winston::RailwayWithRails<TimeSaverRailwayTracks>, winston::Shared_Ptr<TimeSaverRailway>
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
    TimeSaverRailway(const Callbacks callbacks);
    virtual ~TimeSaverRailway() = default;

    static const std::string name();

    using winston::Shared_Ptr<TimeSaverRailway>::Shared;
    using winston::Shared_Ptr<TimeSaverRailway>::make;
private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#endif

#ifndef WINSTON_PLATFORM_TEENSY
BETTER_ENUM(Y2020RailwayTracks, unsigned int,
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
    G,
    G1,
    G2,
    G3
);

class Y2020Railway : public winston::RailwayWithRails<Y2020RailwayTracks>, winston::Shared_Ptr<Y2020Railway>
{
    /*
      //==============Turnout5======================\\
     //                 //                           \\
     ||             Turnout6====C====|               ||
     ||               //                             ||
     ||  |====D====Turnout7                          ||
     ||              //           /=======F====|     ||
     || |====E====Turnout8====Turnout9====G====|     ||
     ||                                              ||
     ||                       --                     ||
     ||                      B                       ||
     \\                     //                       //
      \\====Turnout1==G2==Turnout3==G3==Turnout4====//
                \\                     //
    |====A====Turnout2========G1========
    */

public:
    Y2020Railway(const Callbacks callbacks);
    virtual ~Y2020Railway() = default;

    static const std::string name();

    using winston::Shared_Ptr<Y2020Railway>::Shared;
    using winston::Shared_Ptr<Y2020Railway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2020Railway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2020Railway>::Shared railway;
    };
private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#endif

BETTER_ENUM(Y2021RailwayTracks, unsigned int,
    Turnout1,
    Turnout2,
    Turnout3,
    Turnout4,
    Turnout5,
    Turnout6,
    Turnout7,
    Turnout8,
    Turnout9,
    Turnout10,
    Turnout11,
    Turnout12,
    Turnout13,
    Turnout14,
    Turnout15,
    Turnout16,
    PBF1,
    PBF1a,
    PBF2,
    PBF2a,
    PBF3,
    GBF1,
    GBF2a, GBF2b,
    GBF3a, GBF3b,
    N1,
    N2,
    B1, B2, B3, B4, B5, B6
);
enum class Y2021RailwayDetectors : unsigned int
{
    B1, B4, B3, B6, B2_Speedtrap_A, B2_Speedtrap_B, PBF1
};

class Y2021Railway : public winston::RailwayWithRails<Y2021RailwayTracks>/*, public winston::RailwayWithDetector<Y2021RailwayDetectors>*/, public winston::Shared_Ptr<Y2021Railway>
{
    /*
     //====Turnout11======================B2===============\\
    //           \\                                         \\
    ||  //====Turnout10======B5======Turnout9====Turnout8\\  \\
    ||  ||                          //                \\  \\  \\
    ||  ||                      Turnout12====N2====|   \\ Turnout7
    ||  ||                           //                 \\  ||
    ||  || |====GBF1====Turnout14==Turnout13            ||  ||
    ||  ||                 //         //                ||  ||
    B3  B6              GBF2b        GBF3b              ||  ||
    ||  ||               ||           ||                ||  ||
    ||  ||               Turnout15    ||                ||  ||
    ||  ||               ||     \\    ||                B4  B1
    ||  ||               ||       Turnout16             ||  ||
    ||  ||               ||           ||                ||  ||
    ||   \\             GBF2a        GBF3a              ||  ||
    ||    \\             ||           ||               //   ||
    ||     \\            --           --              //    ||
 Turnout1   \\         //====N1====|                 //    //
    ||  \\   \\       //                            //    //
    \\    Turnout2=Turnout3========PBF3============//    //
     PBF2a====Turnout4========PBF2====================Turnout6
                  \\                                    //
    |====PBF1a====Turnout5========PBF1=================//
    */

public:
    Y2021Railway(const Callbacks callbacks);
    virtual ~Y2021Railway() = default;

    static const std::string name();
    void attachDetectors();

    using winston::Shared_Ptr<Y2021Railway>::Shared;
    using winston::Shared_Ptr<Y2021Railway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2021Railway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2021Railway>::Shared railway;
        std::vector<winston::Locomotive::Shared> locomotiveShed;
    };
private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#ifndef WINSTON_PLATFORM_TEENSY
BETTER_ENUM(SignalRailwayTracks, unsigned int,
    Turnout1,
    Turnout2,
    Turnout3,
    Turnout4,
    Turnout5,
    A,
    B,
    C,
    G1,
    G2,
    G3,
    G4,
    G5,
    G6,
    G7
);

class SignalRailway : public winston::RailwayWithRails<SignalRailwayTracks>, winston::Shared_Ptr<SignalRailway>
{
    /*
      //=====G6=======Turnout5===========G5==========\\
     //                 //                           \\
     ||        |====C====|                           ||
     G7                       --                     G4
     ||                      B                       ||
     \\                     //                       //
      \\====Turnout1==G2==Turnout3==G3==Turnout4====//
                \\                     //
    |====A====Turnout2========G1========
    */

public:
    SignalRailway(const Callbacks callbacks);
    virtual ~SignalRailway() = default;

    static const std::string name();

    using winston::Shared_Ptr<SignalRailway>::Shared;
    using winston::Shared_Ptr<SignalRailway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, public winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<SignalRailway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<SignalRailway>::Shared railway;
    };
private:
    winston::Track::Shared define(const Tracks track);
    void connect();
};
#endif