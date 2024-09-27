#pragma once

#ifdef WINSTON_PLATFORM_TEENSY
//#include "pgmspace.h"
#else
//#define FLASHMEM
#endif

#include <array>
#include "../libwinston/external/better_enum.hpp"
#include "../libwinston/Winston.h"

#ifndef WINSTON_PLATFORM_TEENSY
BETTER_ENUM(MiniRailwayTracks, unsigned int,
    A,
    Turnout1,
    B,
    C
);

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
BETTER_ENUM(SignalTestRailwayTracks, unsigned int,
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

BETTER_ENUM(RailwayWithSidingsTracks, unsigned int,
    A,
    Turnout1,
    B,
    C,
    Turnout2
);
BETTER_ENUM(RailwayWithSidingsSections, unsigned int,
    A, B, C
);

class RailwayWithSiding : public winston::RailwayWithRails<RailwayWithSidingsTracks, winston::RoutesNone, RailwayWithSidingsSections>, winston::Shared_Ptr<RailwayWithSiding>
{
public:
    RailwayWithSiding(const Callbacks callbacks);
    virtual ~RailwayWithSiding() = default;

    static const std::string name();
    const bool supportSections() const;

    using winston::Shared_Ptr<RailwayWithSiding>::Shared;
    using winston::Shared_Ptr<RailwayWithSiding>::make;
private:
    winston::Track::Shared define(const Tracks track);
    Section::Shared define(const Sections section);
    void connect();

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<RailwayWithSiding>::Shared railway);
        virtual winston::Track::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<RailwayWithSiding>::Shared railway;
    };
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
        virtual winston::Track::Shared turnout(const winston::Address address) const;
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
    DoubleSlipTurnout15_16,
    Turnout17,
    Turnout18,
    Turnout19,
    Turnout20,
    PBF1,
    PBF1a,
    PBF2,
    PBF2a,
    PBF3,
    PBF3a,
    GBF1,
    GBF2,
    GBF3a, GBF3b,
    GBF4a, GBF4b,
    N1,
    N2,
    N3,
    B1, B2, B3, B4, B5, B6,
    B_PBF2_PBF1, B_To_GBF,
    PBF_To_N,
    T7_To_T8
);
BETTER_ENUM(Y2021RailwayRoutes, unsigned int,
    B3_PBF1, B3_PBF2, B3_PBF3, B3_N1, B3_N2,
    B6_PBF3, B6_N1, B6_N2
);
BETTER_ENUM(Y2021RailwaySections, unsigned int,
    PBF12, PBF1a, PBF1, PBF2, PBF3,
    N, N1, N2, N3,
    B1, B2, B3, B4, B5, B6,
    GBF, GBF1, GBF2, GBF3, GBF4
);
BETTER_ENUM(RoutesEmpty, unsigned int, None);

enum class Y2021RailwayDetectors : unsigned int
{
    B1, B4, B3, B6, B2_Speedtrap_A, B2_Speedtrap_B, PBF1
};

class Y2021Railway : 
    public winston::RailwayWithRails<Y2021RailwayTracks, Y2021RailwayRoutes, Y2021RailwaySections>,
    public winston::Shared_Ptr<Y2021Railway>
{
    /*
        //====Turnout13==============================B2=======================\\
       //           \\                                                         \\
      // //====Turnout12==============B5==============Turnout11====Turnout10\\  \\
     // ||                                               //              \\  \\  \\
    ||  ||                                        DS_Turnout15====N3====| \\ Turnout9
    ||  ||    |====GBF2====\\                          //                  \\     ||
    ||  || |====GBF1====Turnout18====Turnout17====DS_Turnout16              \\    ||
    ||  ||                             //           //                       \\   ||
    B3  B6                           GBF3b       GBF4b                        ||  ||
    ||  ||                            ||           ||                         ||  ||
    ||  ||                            Turnout19    ||                         ||  ||
    ||  ||                            ||     \\    ||                         B4  B1
    ||  ||                            ||       Turnout20                      ||  ||
    ||  ||                            ||           ||                         ||  ||
    ||  ||                           GBF3a        GBF4a                       ||  ||
    ||  ||                            ||           ||                        //  //
    ||  ||                            --           --                       //  //
    ||   \\                                                                //  //
    ||    \\                                                              //  //
    ||     \\                          //====N2====|                     //  //
 Turnout1   \\         PBF3_To_N====Turnout14====N1====|                //  //
    ||  \\   \\       //                                               //  //
    \\    Turnout2=Turnout3====PBF3=================Turnout6============  //
     \\                                                  \\              //
     PBF2a====Turnout4========PBF2=====================Turnout7====Turnout8
                  \\                                                 //
    |====PBF1a====Turnout5===================PBF1===================//
    */

public:
    Y2021Railway(const Callbacks callbacks);
    virtual ~Y2021Railway() = default;

    static const std::string name();
    void attachDetectors();

    const winston::Result validateFinal();

    const bool supportRoutes() const;
    const bool supportSections() const;

    using winston::Shared_Ptr<Y2021Railway>::Shared;
    using winston::Shared_Ptr<Y2021Railway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2021Railway>::Shared railway);
        virtual winston::Track::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track& track) const;

        virtual winston::Route::Shared route(const winston::Address address) const;
        virtual const winston::Address address(winston::Route::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2021Railway>::Shared railway;
        std::vector<winston::Locomotive::Shared> locomotiveShed;
    };
private:
    winston::Track::Shared define(const Tracks track);
    winston::Route::Shared define(const Routes route);
    Section::Shared define(const Sections section);
    void connect();
};

BETTER_ENUM(Y2024RailwayTracks, unsigned int,
    Turnout1,
    Turnout2,
    Turnout3,
    Turnout4,
    DoubleSlipTurnout5_6,
    Turnout7,
    Turnout8,
    Z1, Z2, Z3,
    PBF1,
    PBF1a,
    PBF2,
    N1,
    N2,
    N3,
    LS1, LS2,
    B1, B2, B3, B4, B5
);
BETTER_ENUM(Y2024RailwayRoutes, unsigned int,
    B5_PBF1, B5_PBF2, B1_PBF1, B1_PBF2
);
BETTER_ENUM(Y2024RailwaySections, unsigned int,
    PBF1, PBF1a, PBF2,
    N1, N2, N3,
    B1, B2, B3, B4, B5,
    LS1, LS2,
    Z
);

class Y2024Railway :
    public winston::RailwayWithRails<Y2024RailwayTracks, Y2024RailwayRoutes, Y2024RailwaySections, 13>,
    public winston::Shared_Ptr<Y2024Railway>
{
    /*
    *   // ========= B4 ======== Turnout4 ==== B3 ===== \\
    *  //                           \\                   \\
    * //                             Z1                   B2
    * ||                              \\                  ||
    * ||                  N1 ==== Turnout5_6 ==== N2      ||
    * ||                                \\                ||      
    * || LS2 ===== \\                    Z2               ||
    * B5            \\                    \\              ||
    * || LS1 ==== Turnout8 ==== Z3 ==== Turnout7 ==== N3  || 
    * ||                                                  ||  
    * \\            // ==== PBF1 ==== Turnout2 ==== PBF1a //
    *  \\          //        B1           \\             B1
    *   \\ ==== Turnout1 ==== PBF2 ==== Turnout3 =======//
    */

public:
    Y2024Railway(const Callbacks callbacks);
    virtual ~Y2024Railway() = default;

    static const std::string name();
    void attachDetectors();

    const winston::Result validateFinal();

    const bool supportRoutes() const;
    const bool supportSections() const;
    const bool supportSegments() const;

    using winston::Shared_Ptr<Y2024Railway>::Shared;
    using winston::Shared_Ptr<Y2024Railway>::make;

public:
    class AddressTranslator : public winston::DigitalCentralStation::TurnoutAddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2024Railway>::Shared railway);
        virtual winston::Track::Shared turnout(const winston::Address address) const;
        virtual const winston::Address address(winston::Track& track) const;

        virtual winston::Route::Shared route(const winston::Address address) const;
        virtual const winston::Address address(winston::Route::Shared track) const;

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2024Railway>::Shared railway;
        std::vector<winston::Locomotive::Shared> locomotiveShed;
    };
private:
    winston::Track::Shared define(const Tracks track);
    winston::Route::Shared define(const Routes route);
    Section::Shared define(const Sections section);
    Segment::Shared define(const Segments segment);
    void connect();
};