
#include <string>
#include "../libwinston/Library.h"
#include "railways.h"
#include "../libwinston/better_enum.hpp"

#define BUMPER(track) case Tracks::_enumerated::track: return winston::Bumper::make(#track); 
#define RAIL(track) case Tracks::_enumerated::track: return winston::Rail::make(#track); 
#define TURNOUT(track, callback, ...) case Tracks::_enumerated::track: return winston::Turnout::make(#track, callback, __VA_ARGS__); 

#ifndef WINSTON_PLATFORM_TEENSY
MiniRailway::MiniRailway(const Callbacks callbacks) : winston::RailwayWithRails<MiniRailwayTracks>(callbacks) {};

winston::Track::Shared MiniRailway::define(const Tracks track)
{
    switch (track) {
        BUMPER(A);
        BUMPER(B);
        BUMPER(C);
    case Tracks::Turnout1:
        return winston::Turnout::make(std::string("Turnout1"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void MiniRailway::connect()
{
    this->track(Tracks::A)->connect(winston::Track::Connection::A, this->track(Tracks::Turnout1), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::B), winston::Track::Connection::A);
    this->track(Tracks::Turnout1)->connect(winston::Track::Connection::C, this->track(Tracks::C), winston::Track::Connection::A);

    this->track(Tracks::B)->attachSignal(winston::SignalH::make(), winston::Track::Connection::A);
    this->track(Tracks::C)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
}

const std::string MiniRailway::name()
{
    return std::string("MiniRailway");
}

SignalTestRailway::SignalTestRailway(const Callbacks callbacks) : winston::RailwayWithRails<SignalTestRailwayTracks>(callbacks) {};

winston::Track::Shared SignalTestRailway::define(const Tracks track)
{
    switch (track) {
    BUMPER(A);
    BUMPER(B);
    BUMPER(C);
    BUMPER(D);
    BUMPER(F);
    BUMPER(G);
    BUMPER(J);
    BUMPER(K);
    BUMPER(N);
    BUMPER(Q);
    BUMPER(R);
    BUMPER(U);
    BUMPER(W);
    BUMPER(L0);
    BUMPER(L8);
    RAIL(E);
    RAIL(H);
    RAIL(I);
    RAIL(L);
    RAIL(M);
    RAIL(O);
    RAIL(P);
    RAIL(S);
    RAIL(T);
    RAIL(V);
    RAIL(L1);
    RAIL(L2);
    RAIL(L3);
    RAIL(L4);
    RAIL(L5);
    RAIL(L6);
    RAIL(L7);
    TURNOUT(Turnout1, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    TURNOUT(Turnout2, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    TURNOUT(Turnout3, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void SignalTestRailway::connect()
{
    this->track(Tracks::A)->connect(winston::Track::Connection::A, this->track(Tracks::Turnout1), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::B), winston::Track::Connection::A);
    this->track(Tracks::Turnout1)->connect(winston::Track::Connection::C, this->track(Tracks::C), winston::Track::Connection::A);
    this->track(Tracks::B)->attachSignal(winston::SignalH::make(), winston::Track::Connection::A);
    this->track(Tracks::C)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);

    this->track(Tracks::D)->connect(winston::Track::Connection::A, this->track(Tracks::E), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::F), winston::Track::Connection::A);
    this->track(Tracks::F)->attachSignal(winston::SignalV::make(), winston::Track::Connection::A);
    this->track(Tracks::E)->attachSignal(winston::SignalH::make(), winston::Track::Connection::A);

    this->track(Tracks::G)->connect(winston::Track::Connection::A, this->track(Tracks::H), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::I), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::J), winston::Track::Connection::A);
    this->track(Tracks::H)->attachSignal(winston::SignalH::make(), winston::Track::Connection::A);
    this->track(Tracks::I)->attachSignal(winston::SignalHV::make(), winston::Track::Connection::A);
    this->track(Tracks::J)->attachSignal(winston::SignalV::make(), winston::Track::Connection::A);

    this->track(Tracks::K)->connect(winston::Track::Connection::A, this->track(Tracks::L), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::M), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::N), winston::Track::Connection::A);
    this->track(Tracks::L)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::M)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::N)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);

    this->track(Tracks::Q)->connect(winston::Track::Connection::A, this->track(Tracks::Turnout2), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::P), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::O), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::Turnout2), winston::Track::Connection::C);
    this->track(Tracks::Q)->attachSignal(winston::SignalHV::make(), winston::Track::Connection::A);

    this->track(Tracks::R)->connect(winston::Track::Connection::A, this->track(Tracks::S), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::Turnout3), winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, this->track(Tracks::T), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::U), winston::Track::Connection::A);
    this->track(Tracks::Turnout3)->connect(winston::Track::Connection::C, this->track(Tracks::V), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::W), winston::Track::Connection::A);
    this->track(Tracks::R)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::S)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::S)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::B);
    this->track(Tracks::T)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::T)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::B);
    this->track(Tracks::U)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::V)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::V)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::B);
    this->track(Tracks::W)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);

    this->track(Tracks::L0)->connect(winston::Track::Connection::A, this->track(Tracks::L1), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L2), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L3), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L4), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L5), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L6), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L7), winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, this->track(Tracks::L8), winston::Track::Connection::A);
    this->track(Tracks::L0)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::L1)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::B);
    this->track(Tracks::L4)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::L7)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
    this->track(Tracks::L8)->attachSignal(winston::SignalKS::make(), winston::Track::Connection::A);
}

const std::string SignalTestRailway::name()
{
    return std::string("SignalTestRailway");
}

RailwayWithSiding::RailwayWithSiding(const Callbacks callbacks) : winston::RailwayWithRails<RailwayWithSidingsTracks>(callbacks) {};
RailwayWithSiding::AddressTranslator::AddressTranslator(RailwayWithSiding::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared RailwayWithSiding::AddressTranslator::turnout(const winston::Address address) const
{
    switch (address)
    {
    default:
    case 0: return std::static_pointer_cast<winston::Turnout>(railway->track(Tracks::Turnout1)); break;
    case 1: return std::static_pointer_cast<winston::Turnout>(railway->track(Tracks::Turnout2)); break;
    }
}

const winston::Address RailwayWithSiding::AddressTranslator::address(winston::Track::Shared track) const
{
    switch (railway->trackEnum(track))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    }
    return 0;
}

winston::Track::Shared RailwayWithSiding::define(const Tracks track)
{
    switch (track)
    {
    case Tracks::A:
    case Tracks::B:
    case Tracks::C:
        return winston::Rail::make();
    case Tracks::Turnout1:
    case Tracks::Turnout2:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, track == +Tracks::Turnout2);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void RailwayWithSiding::connect()
{
    auto a = this->track(Tracks::A);
    auto b = this->track(Tracks::B);
    auto c = this->track(Tracks::C);
    auto t1 = this->track(Tracks::Turnout1);
    auto t2 = this->track(Tracks::Turnout2);

    a->connect(winston::Track::Connection::A, t1, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, b, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t2, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, a, winston::Track::Connection::B);
    t1->connect(winston::Track::Connection::C, c, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t2, winston::Track::Connection::B);
}

const std::string RailwayWithSiding::name()
{
    return std::string("RailwayWithSiding");
}

TimeSaverRailway::TimeSaverRailway(const Callbacks callbacks) : winston::RailwayWithRails<TimeSaverRailwayTracks>(callbacks) {};
winston::Track::Shared TimeSaverRailway::define(const Tracks track)
{
    switch (track)
    {
    case Tracks::A:
    case Tracks::B:
    case Tracks::C:
    case Tracks::D:
    case Tracks::E:
        return winston::Bumper::make();
    case Tracks::Turnout1:
    case Tracks::Turnout3:
    case Tracks::Turnout4:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout2:
    case Tracks::Turnout5:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void TimeSaverRailway::connect()
{
    // counter-wise orientation
    auto a = this->track(Tracks::A);
    auto b = this->track(Tracks::B);
    auto c = this->track(Tracks::C);
    auto d = this->track(Tracks::D);
    auto e = this->track(Tracks::E);
    auto t1 = this->track(Tracks::Turnout1);
    auto t2 = this->track(Tracks::Turnout2);
    auto t3 = this->track(Tracks::Turnout3);
    auto t4 = this->track(Tracks::Turnout4);
    auto t5 = this->track(Tracks::Turnout5);

    a->connect(winston::Track::Connection::A, t1, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, b, winston::Track::Connection::A);

    t1->connect(winston::Track::Connection::C, t2, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t3, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, c, winston::Track::Connection::A);

    t2->connect(winston::Track::Connection::C, t5, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, d, winston::Track::Connection::A);

    t3->connect(winston::Track::Connection::B, t4, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t5, winston::Track::Connection::B);

    t4->connect(winston::Track::Connection::C, e, winston::Track::Connection::A);
}
    
const std::string TimeSaverRailway::name()
{
    return std::string("TimeSaverRailway");
}

Y2020Railway::Y2020Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2020RailwayTracks>(callbacks) {};
const std::string Y2020Railway::name()
{
    return std::string("Y2020Railway");
}

Y2020Railway::AddressTranslator::AddressTranslator(Y2020Railway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared Y2020Railway::AddressTranslator::turnout(const winston::Address address) const
{
	Tracks track = Tracks::Turnout1;
    switch (address)
    {
    default:
    case 0: track = Tracks::Turnout1; break;
    case 1: track = Tracks::Turnout2; break;
    case 2: track = Tracks::Turnout3; break;
    case 3: track = Tracks::Turnout4; break;
    case 4: track = Tracks::Turnout5; break;
    case 5: track = Tracks::Turnout6; break;
    case 6: track = Tracks::Turnout7; break;
    case 7: track = Tracks::Turnout8; break;
    case 8: track = Tracks::Turnout9; break;
    }
    return std::static_pointer_cast<winston::Turnout>(railway->track(track));
}

const winston::Address Y2020Railway::AddressTranslator::address(winston::Track::Shared track) const
{
    switch (railway->trackEnum(track))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    case Tracks::Turnout3: return 2; break;
    case Tracks::Turnout4: return 3; break;
    case Tracks::Turnout5: return 4; break;
    case Tracks::Turnout6: return 5; break;
    case Tracks::Turnout7: return 6; break;
    case Tracks::Turnout8: return 7; break;
    case Tracks::Turnout9: return 8; break;
    }
    return 0;
}

winston::Track::Shared Y2020Railway::define(const Tracks track)
{
    switch (track)
    {
    case Tracks::A:
    case Tracks::B:
    case Tracks::C:
    case Tracks::D:
    case Tracks::E:
    case Tracks::F:
    case Tracks::G:
        return winston::Bumper::make();
    case Tracks::G1:
    case Tracks::G2:
    case Tracks::G3:
        return winston::Rail::make();
    case Tracks::Turnout1:
    case Tracks::Turnout2:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout3:
    case Tracks::Turnout4:
    case Tracks::Turnout5:
    case Tracks::Turnout6:
    case Tracks::Turnout7:
    case Tracks::Turnout8:
    case Tracks::Turnout9:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }

}

void Y2020Railway::connect()
{
    auto a = this->track(Tracks::A);
    auto b = this->track(Tracks::B);
    auto c = this->track(Tracks::C);
    auto d = this->track(Tracks::D);
    auto e = this->track(Tracks::E);
    auto f = this->track(Tracks::F);
    auto g = this->track(Tracks::G);
    auto g1 = this->track(Tracks::G1);
    auto g2 = this->track(Tracks::G2);
    auto g3 = this->track(Tracks::G3);
    auto t1 = this->track(Tracks::Turnout1);
    auto t2 = this->track(Tracks::Turnout2);
    auto t3 = this->track(Tracks::Turnout3);
    auto t4 = this->track(Tracks::Turnout4);
    auto t5 = this->track(Tracks::Turnout5);
    auto t6 = this->track(Tracks::Turnout6);
    auto t7 = this->track(Tracks::Turnout7);
    auto t8 = this->track(Tracks::Turnout8);
    auto t9 = this->track(Tracks::Turnout9);

    a->connect(winston::Track::Connection::A, t2, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g1, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t4, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t5, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t1, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::C, t2, winston::Track::Connection::C);

    t1->connect(winston::Track::Connection::B, g2, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t4, winston::Track::Connection::C);

    t3->connect(winston::Track::Connection::C, b, winston::Track::Connection::A);

    t5->connect(winston::Track::Connection::C, t6, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, t7, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::C, t8, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, e, winston::Track::Connection::A);

    t6->connect(winston::Track::Connection::B, c, winston::Track::Connection::A);
    t7->connect(winston::Track::Connection::B, d, winston::Track::Connection::A);

    t8->connect(winston::Track::Connection::B, t9, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g, winston::Track::Connection::A);
    t9->connect(winston::Track::Connection::C, f, winston::Track::Connection::A);

#define attachSignalY2020(track, SignalClass, guardedConnection) \
    track->attachSignal(SignalClass::make([=](const winston::Signal::Aspects aspect)->const winston::State { return this->callbacks.signalUpdateCallback(track, guardedConnection, aspect); }), guardedConnection);

    attachSignalY2020(g1, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g1, winston::SignalKS, winston::Track::Connection::B);
    attachSignalY2020(a, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(b, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g2, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g3, winston::SignalKS, winston::Track::Connection::B);
}
#endif

Y2021Railway::Y2021Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2021RailwayTracks>(callbacks){};
const std::string Y2021Railway::name()
{
    return std::string("Y2021Railway");
}

Y2021Railway::AddressTranslator::AddressTranslator(Y2021Railway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared Y2021Railway::AddressTranslator::turnout(const winston::Address address) const
{
    Tracks track = Tracks::Turnout1;
    switch (address)
    {
    default:
    case 0: track = Tracks::Turnout1; break;
    case 1: track = Tracks::Turnout2; break;
    case 2: track = Tracks::Turnout3; break;
    case 3: track = Tracks::Turnout4; break;
    case 4: track = Tracks::Turnout5; break;
    case 5: track = Tracks::Turnout6; break;
    case 6: track = Tracks::Turnout7; break;
    case 7: track = Tracks::Turnout8; break;
    case 8: track = Tracks::Turnout9; break;
    case 9: track = Tracks::Turnout10; break;
    case 10: track = Tracks::Turnout11; break;
    case 11: track = Tracks::Turnout12; break;
    case 12: track = Tracks::Turnout13; break;
    case 13: track = Tracks::Turnout14; break;
    case 14: track = Tracks::Turnout15; break;
    case 15: track = Tracks::Turnout16; break;
    }
    return std::static_pointer_cast<winston::Turnout>(railway->track(track));
}

const winston::Address Y2021Railway::AddressTranslator::address(winston::Track::Shared track) const
{
    switch (railway->trackEnum(track))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    case Tracks::Turnout3: return 2; break;
    case Tracks::Turnout4: return 3; break;
    case Tracks::Turnout5: return 4; break;
    case Tracks::Turnout6: return 5; break;
    case Tracks::Turnout7: return 6; break;
    case Tracks::Turnout8: return 7; break;
    case Tracks::Turnout9: return 8; break;
    case Tracks::Turnout10: return 9; break;
    case Tracks::Turnout11: return 10; break;
    case Tracks::Turnout12: return 11; break;
    case Tracks::Turnout13: return 12; break;
    case Tracks::Turnout14: return 13; break;
    case Tracks::Turnout15: return 14; break;
    case Tracks::Turnout16: return 15; break;
    }
    return 0;
}

winston::Track::Shared Y2021Railway::define(const Tracks track)
{
    switch (track)
    {
        BUMPER(N1);
        BUMPER(N2);
        BUMPER(GBF1);
        BUMPER(GBF2a);
        BUMPER(GBF3a);
        BUMPER(PBF1a);
        RAIL(PBF1);
        RAIL(PBF2);
        RAIL(PBF2a);
        RAIL(PBF3);
        RAIL(GBF2b);
        RAIL(GBF3b);
        RAIL(B1);
        RAIL(B2);
        RAIL(B3);
        RAIL(B4);
        RAIL(B5);
        RAIL(B6);
    case Tracks::Turnout1:
    case Tracks::Turnout3:
    case Tracks::Turnout7:
    case Tracks::Turnout9:
    case Tracks::Turnout12:
    case Tracks::Turnout14:
        return winston::Turnout::make(std::string(track._to_string()), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout2:
    case Tracks::Turnout4:
    case Tracks::Turnout5:
    case Tracks::Turnout6:
    case Tracks::Turnout8:
    case Tracks::Turnout10:
    case Tracks::Turnout11:
    case Tracks::Turnout13:
    case Tracks::Turnout15:
    case Tracks::Turnout16:
        return winston::Turnout::make(std::string(track._to_string()), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }

}

void Y2021Railway::connect()
{
#define LOCAL_TRACK(var)  auto var = this->track(Tracks::var);
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2a);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(PBF3);
    LOCAL_TRACK(GBF1);
    LOCAL_TRACK(GBF2a);
    LOCAL_TRACK(GBF2b);
    LOCAL_TRACK(GBF3a);
    LOCAL_TRACK(GBF3b);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(Turnout1);
    LOCAL_TRACK(Turnout2);
    LOCAL_TRACK(Turnout3);
    LOCAL_TRACK(Turnout4);
    LOCAL_TRACK(Turnout5);
    LOCAL_TRACK(Turnout6);
    LOCAL_TRACK(Turnout7);
    LOCAL_TRACK(Turnout8);
    LOCAL_TRACK(Turnout9);
    LOCAL_TRACK(Turnout10);
    LOCAL_TRACK(Turnout11);
    LOCAL_TRACK(Turnout12);
    LOCAL_TRACK(Turnout13);
    LOCAL_TRACK(Turnout14);
    LOCAL_TRACK(Turnout15);
    LOCAL_TRACK(Turnout16);

    const auto A = winston::Track::Connection::A;
    const auto B = winston::Track::Connection::B;
    const auto C = winston::Track::Connection::C;

    size_t device = 0;
    size_t port = 0;
    /*
    // outer loop
    Turnout1->connect(B, PBF2a, A)
        ->connect(B, Turnout4, A)
        ->connect(B, PBF2, A)
        ->connect(B, H(5, port, device), Turnout6, C)
        ->connect(A, B1, A)
        ->connect(B, Turnout7, A)
        ->connect(B, B2, A)
        ->connect(B, Turnout11, B)
        ->connect(A, B3, A)
        ->connect(B, KS_dummy(), Turnout1, A);

    // inner loop
    Turnout2->connect(A, Turnout3, A)
        ->connect(B, PBF3, A)
        ->connect(B, H(5, port, device), B4, A)
        ->connect(B, Turnout8, C)
        ->connect(A, Turnout9, A)
        ->connect(B, B5, A)
        ->connect(B, Turnout10, A)
        ->connect(B, B6, A)
        ->connect(B, KS_dummy(), Turnout2, C);

    // lower track
    PBF1a->connect(A, KS_dummy(0), Turnout5, B)
        ->connect(A, PBF1, A)
        ->connect(B, H(5, port, device), Turnout6, B);

    // inner turnouts
    Turnout1->connect(C, Turnout2, B);
    Turnout4->connect(C, Turnout5, C);
    Turnout7->connect(C, Turnout8, B);
    Turnout9->connect(C, Turnout12, C);
    Turnout10->connect(C, Turnout11, C);
    Turnout15->connect(C, Turnout16, C);

    // nebengleise
    Turnout3->connect(C, N1, A, KS_dummy(0));
    Turnout12->connect(B, N2, A, KS_dummy(0));

    // GBF
   Turnout12->connect(A, Turnout13, A)
        ->connect(B, Turnout14, A)
        ->connect(B, GBF1, A, KS_dummy(0));
   Turnout13->connect(C, GBF3b, A, KS_dummy(0))
       ->connect(B, KS_dummy(0), Turnout16, A)
       ->connect(B, GBF3a, A, KS_dummy(0));
   Turnout14->connect(C, GBF2b, A, KS_dummy(0))
       ->connect(B, KS_dummy(0), Turnout15, B)
       ->connect(A, GBF2a, A, KS_dummy(0));*/

       // outer loop
    Turnout1->connect(B, PBF2a, A)
        ->connect(B, Turnout4, A)
        ->connect(B, PBF2, A)
        ->connect(B, Turnout6, C)
        ->connect(A, B1, A)
        ->connect(B, Turnout7, A)
        ->connect(B, B2, A)
        ->connect(B, Turnout11, B)
        ->connect(A, B3, A)
        ->connect(B, Turnout1, A);

    // inner loop
    Turnout2->connect(A, Turnout3, A)
        ->connect(B, PBF3, A)
        ->connect(B, B4, A)
        ->connect(B, Turnout8, C)
        ->connect(A, Turnout9, A)
        ->connect(B, B5, A)
        ->connect(B, Turnout10, A)
        ->connect(B, B6, A)
        ->connect(B, Turnout2, C);

    // lower track
    PBF1a->connect(A, Turnout5, B)
        ->connect(A, PBF1, A)
        ->connect(B, Turnout6, B);

    // inner turnouts
    Turnout1->connect(C, Turnout2, B);
    Turnout4->connect(C, Turnout5, C);
    Turnout7->connect(C, Turnout8, B);
    Turnout9->connect(C, Turnout12, C);
    Turnout10->connect(C, Turnout11, C);
    Turnout15->connect(C, Turnout16, C);

    // nebengleise
    Turnout3->connect(C, N1, A);
    Turnout12->connect(B, N2, A);

    // GBF
    Turnout12->connect(A, Turnout13, A)
        ->connect(B, Turnout14, A)
        ->connect(B, GBF1, A);
    Turnout13->connect(C, GBF3b, A)
        ->connect(B, Turnout16, A)
        ->connect(B, GBF3a, A);
    Turnout14->connect(C, GBF2b, A)
        ->connect(B, Turnout15, B)
        ->connect(A, GBF2a, A);
}

void Y2021Railway::attachDetectors()
{
}

#ifndef WINSTON_PLATFORM_TEENSY
SignalRailway::SignalRailway(const Callbacks callbacks) : winston::RailwayWithRails<SignalRailwayTracks>(callbacks) {};
const std::string SignalRailway::name()
{
    return std::string("SignalRailway");
}

SignalRailway::AddressTranslator::AddressTranslator(SignalRailway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared SignalRailway::AddressTranslator::turnout(const winston::Address address) const
{
    Tracks track = Tracks::Turnout1;
    switch (address)
    {
    default:
    case 0: track = Tracks::Turnout1; break;
    case 1: track = Tracks::Turnout2; break;
    case 2: track = Tracks::Turnout3; break;
    case 3: track = Tracks::Turnout4; break;
    case 4: track = Tracks::Turnout5; break;
    }
    return std::static_pointer_cast<winston::Turnout>(railway->track(track));
}

const winston::Address SignalRailway::AddressTranslator::address(winston::Track::Shared track) const
{
    switch (railway->trackEnum(track))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    case Tracks::Turnout3: return 2; break;
    case Tracks::Turnout4: return 3; break;
    case Tracks::Turnout5: return 4; break;
    }
    return 0;
}

winston::Track::Shared SignalRailway::define(const Tracks track)
{
    switch (track)
    {
        BUMPER(A);
        BUMPER(B);
        BUMPER(C);
        RAIL(G1);
        RAIL(G2);
        RAIL(G3);
        RAIL(G4);
        RAIL(G5);
        RAIL(G6);
        RAIL(G7);
    /*case Tracks::A:
    //case Tracks::B:
    //case Tracks::C:
    //    return winston::Bumper::make();
    case Tracks::G1:
    case Tracks::G2:
    case Tracks::G3:
    case Tracks::G4:
    case Tracks::G5:
    case Tracks::G6:
    case Tracks::G7:
        return winston::Rail::make();*/
    case Tracks::Turnout1: return winston::Turnout::make(std::string("Turnout1"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout2: return winston::Turnout::make(std::string("Turnout2"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout3: return winston::Turnout::make(std::string("Turnout3"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout4: return winston::Turnout::make(std::string("Turnout4"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout5: return winston::Turnout::make(std::string("Turnout5"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::static_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }

}

void SignalRailway::connect()
{
    auto a = this->track(Tracks::A);
    auto b = this->track(Tracks::B);
    auto c = this->track(Tracks::C);
    auto g1 = this->track(Tracks::G1);
    auto g2 = this->track(Tracks::G2);
    auto g3 = this->track(Tracks::G3);
    auto g4 = this->track(Tracks::G4);
    auto g5 = this->track(Tracks::G5);
    auto g6 = this->track(Tracks::G6);
    auto g7 = this->track(Tracks::G7);
    auto t1 = this->track(Tracks::Turnout1);
    auto t2 = this->track(Tracks::Turnout2);
    auto t3 = this->track(Tracks::Turnout3);
    auto t4 = this->track(Tracks::Turnout4);
    auto t5 = this->track(Tracks::Turnout5);

    /*auto KS = [distance, =](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
        return winston::SignalKS::make(distance, [=](const winston::Signal::Aspects aspect)->const winston::State {
            return this->callbacks.signalUpdateCallback(track, connection, aspect);
          });
    };*/

    a->connect(winston::Track::Connection::A, t2, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g1, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t4, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, g4, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g5, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t5, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g6, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g7, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, t1, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::C, t2, winston::Track::Connection::C);

    t1->connect(winston::Track::Connection::B, g2, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, t4, winston::Track::Connection::B);

    t3->connect(winston::Track::Connection::C, b, winston::Track::Connection::A);

    t5->connect(winston::Track::Connection::C, c, winston::Track::Connection::A);
    /*a->connect(winston::Track::Connection::A, KS_dummy(0), t2, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g1, winston::Track::Connection::B, KS_dummy(0))
        ->connect(winston::Track::Connection::A, KS_dummy(0), t4, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, g4, winston::Track::Connection::B, KS_dummy(0))
        ->connect(winston::Track::Connection::A, g5, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, KS_dummy(0), t5, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g6, winston::Track::Connection::B, KS_dummy(0))
        ->connect(winston::Track::Connection::A, g7, winston::Track::Connection::B, KS_dummy(0))
        ->connect(winston::Track::Connection::A, t1, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::C, t2, winston::Track::Connection::C);

    t1->connect(winston::Track::Connection::B, g2, winston::Track::Connection::A, KS_dummy(0))
        ->connect(winston::Track::Connection::B, t3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, KS_dummy(0), t4, winston::Track::Connection::B);

    t3->connect(winston::Track::Connection::C, b, winston::Track::Connection::A, KS_dummy(0));

    t5->connect(winston::Track::Connection::C, c, winston::Track::Connection::A, KS_dummy(0));*/

#define attachSignalSR(track, SignalClass, guardedConnection) \
    track->attachSignal(SignalClass::make([=](const winston::Signal::Aspects aspect)->const winston::State { return this->callbacks.signalUpdateCallback(track, guardedConnection, aspect); }), guardedConnection);

    attachSignalSR(g1, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g1, winston::SignalKS, winston::Track::Connection::B);
    attachSignalSR(a, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(b, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(c, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g2, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g3, winston::SignalKS, winston::Track::Connection::B);
    attachSignalSR(g4, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g4, winston::SignalKS, winston::Track::Connection::B);
    attachSignalSR(g5, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g5, winston::SignalKS, winston::Track::Connection::B);
    attachSignalSR(g6, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g6, winston::SignalKS, winston::Track::Connection::B);
    attachSignalSR(g7, winston::SignalKS, winston::Track::Connection::A);
    attachSignalSR(g7, winston::SignalKS, winston::Track::Connection::B);
}
#endif