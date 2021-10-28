#pragma once

#include <array>
#include "../libwinston/Winston.h"

enum class MiniRailwayTracks : unsigned int
{
    A,
    Turnout1,
    B,
    C
};

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
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);
};

enum class SignalTestRailwayTracks : unsigned int
{
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
    L0, L1, L2, L20, L2S, L23, L3, L4, L5
};

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
    * |====L0====KL0a=KL1b====L1====L2====L20====L2S=KL2Sa====L23====L3====L4====KL4a=KL5a====L5====|
    */

public:

    SignalTestRailway(const Callbacks callbacks);
    virtual ~SignalTestRailway() = default;

    static const std::string name();

    using winston::Shared_Ptr<SignalTestRailway>::Shared;
    using winston::Shared_Ptr<SignalTestRailway>::make;

private:
    winston::Track::Shared define(const Tracks track);
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);
};

/*
    /--A--\
    |     |
    \-2-B-1
      \-C-/

A-Turnout1-B-Turnout2-A
          \C/

*/

enum class RailwayWithSidingsTracks : unsigned int
{
    A,
    Turnout1,
    B,
    C,
    Turnout2
};

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
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);

public:
    class AddressTranslator : public winston::DigitalCentralStation::AddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<RailwayWithSiding>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address);
        virtual winston::Locomotive::Shared locomotive(const winston::Address address);
        virtual const winston::Address address(winston::Track::Shared track);

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<RailwayWithSiding>::Shared railway;
    };
};

enum class TimeSaverRailwayTracks : unsigned int
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
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);
};

enum class Y2020RailwayTracks : unsigned int
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
    G,
    G1,
    G2,
    G3
};

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
    class AddressTranslator : public winston::DigitalCentralStation::AddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<Y2020Railway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address);
        virtual winston::Locomotive::Shared locomotive(const winston::Address address);
        virtual const winston::Address address(winston::Track::Shared track);

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<Y2020Railway>::Shared railway;
    };
private:
    winston::Track::Shared define(const Tracks track);
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);
};

enum class SignalRailwayTracks : unsigned int
{
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
};

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
    class AddressTranslator : public winston::DigitalCentralStation::AddressTranslator, winston::Shared_Ptr<AddressTranslator>
    {
    public:
        AddressTranslator(winston::Shared_Ptr<SignalRailway>::Shared railway);
        virtual winston::Turnout::Shared turnout(const winston::Address address);
        virtual winston::Locomotive::Shared locomotive(const winston::Address address);
        virtual const winston::Address address(winston::Track::Shared track);

        using Shared_Ptr<AddressTranslator>::Shared;
        using Shared_Ptr<AddressTranslator>::make;

    private:
        winston::Shared_Ptr<SignalRailway>::Shared railway;
    };
private:
    winston::Track::Shared define(const Tracks track);
    void connect(std::array < winston::Track::Shared, tracksCount()>& tracks);
};
