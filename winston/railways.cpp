#ifdef WINSTON_PLATFORM_TEENSY
#define Binary_h
// keeps binary_h from beeing used which kills our 
#endif

#include <string>
#include "../libwinston/Library.h"
#include "railways.h"
#include "../libwinston/external/better_enum.hpp"

#define BUMPER(track, ...) case Tracks::_enumerated::track: return winston::Bumper::make(#track, (winston::Id)Tracks::_enumerated::track, __VA_ARGS__); 
#define RAIL(track, ...) case Tracks::_enumerated::track: return winston::Rail::make(#track, (winston::Id)Tracks::_enumerated::track, __VA_ARGS__); 
#define TURNOUT(track, callback, ...) case Tracks::_enumerated::track: return winston::Turnout::make(#track, (winston::Id)Tracks::_enumerated::track, callback, __VA_ARGS__); 
#define DOUBLESLIPTURNOUT(track, callback, ...) case Tracks::_enumerated::track: return winston::DoubleSlipTurnout::make(#track, (winston::Id)Tracks::_enumerated::track, callback, __VA_ARGS__); 

#ifndef WINSTON_PLATFORM_TEENSY
MiniRailway::MiniRailway(const Callbacks callbacks) : winston::RailwayWithRails<MiniRailwayTracks>(callbacks) {};

winston::Track::Shared MiniRailway::define(const Tracks track)
{
    switch (track) {
        BUMPER(A);
        BUMPER(B);
        BUMPER(C);
    case Tracks::Turnout1:
        return winston::Turnout::make(std::string("Turnout1"), (winston::Id)Tracks::Turnout1, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); }, false);
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
    BUMPER(A, winston::library::track::Roco::G1);
    BUMPER(B, winston::library::track::Roco::G1);
    BUMPER(C, winston::library::track::Roco::G1);
    BUMPER(D, winston::library::track::Roco::G1);
    BUMPER(F, winston::library::track::Roco::G1);
    BUMPER(G, winston::library::track::Roco::G1);
    BUMPER(J, winston::library::track::Roco::G1);
    BUMPER(K, winston::library::track::Roco::G1);
    BUMPER(N, winston::library::track::Roco::G1);
    BUMPER(Q, winston::library::track::Roco::G1);
    BUMPER(R, winston::library::track::Roco::G1);
    BUMPER(U, winston::library::track::Roco::G1);
    BUMPER(W, winston::library::track::Roco::G1);
    BUMPER(L0, winston::library::track::Roco::G1);
    BUMPER(L8, winston::library::track::Roco::G1);
    RAIL(E, winston::library::track::Roco::G1);
    RAIL(H, winston::library::track::Roco::G1);
    RAIL(I, winston::library::track::Roco::G1);
    RAIL(L, winston::library::track::Roco::G1);
    RAIL(M, winston::library::track::Roco::G1);
    RAIL(O, winston::library::track::Roco::G1);
    RAIL(P, winston::library::track::Roco::G1);
    RAIL(S, winston::library::track::Roco::G1);
    RAIL(T, winston::library::track::Roco::G1);
    RAIL(V, winston::library::track::Roco::G1);
    RAIL(L1, winston::library::track::Roco::G1);
    RAIL(L2, winston::library::track::Roco::G1);
    RAIL(L3, winston::library::track::Roco::G1);
    RAIL(L4, winston::library::track::Roco::G1);
    RAIL(L5, winston::library::track::Roco::G1);
    RAIL(L6, winston::library::track::Roco::G1);
    RAIL(L7, winston::library::track::Roco::G1);
    TURNOUT(Turnout1, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); });
    TURNOUT(Turnout2, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); });
    TURNOUT(Turnout3, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); });
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

RailwayWithSiding::RailwayWithSiding(const Callbacks callbacks) : winston::RailwayWithRails<RailwayWithSidingsTracks, winston::RoutesNone, RailwayWithSidingsSections>(callbacks) {};
RailwayWithSiding::AddressTranslator::AddressTranslator(RailwayWithSiding::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Track::Shared RailwayWithSiding::AddressTranslator::turnout(const winston::Address address) const
{
    switch (address)
    {
    default:
    case 0: return railway->track(Tracks::Turnout1); break;
    case 1: return railway->track(Tracks::Turnout2); break;
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
        return winston::Turnout::make(std::string(""), (winston::Id)track, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); }, track == +Tracks::Turnout2);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

RailwayWithSiding::Section::Shared RailwayWithSiding::define(const RailwayWithSiding::Sections section)
{
    auto a = this->track(Tracks::A);
    auto b = this->track(Tracks::B);
    auto c = this->track(Tracks::C);
    auto t1 = this->track(Tracks::Turnout1);
    auto t2 = this->track(Tracks::Turnout2);

    switch (section)
    {
    case RailwayWithSiding::Sections::A:
        return RailwayWithSiding::Section::make(RailwayWithSiding::Sections::A, winston::Section::Type::Free, winston::TrackSet({ a, t1 }));
    case RailwayWithSiding::Sections::B:
        return RailwayWithSiding::Section::make(RailwayWithSiding::Sections::B, winston::Section::Type::Free, winston::TrackSet({ b }));
    case RailwayWithSiding::Sections::C:
        return RailwayWithSiding::Section::make(RailwayWithSiding::Sections::C, winston::Section::Type::Free, winston::TrackSet({ c, t2 }));
    default:
        winston::hal::fatal(std::string("section ") + std::string(section._to_string()) + std::string("not in switch"));
        return nullptr;
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

const bool RailwayWithSiding::supportSections() const
{
    return true;
}

Y2020Railway::Y2020Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2020RailwayTracks>(callbacks) {};
const std::string Y2020Railway::name()
{
    return std::string("Y2020Railway");
}

Y2020Railway::AddressTranslator::AddressTranslator(Y2020Railway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Track::Shared Y2020Railway::AddressTranslator::turnout(const winston::Address address) const
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
    return railway->track(track);
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
        return winston::Turnout::make(std::string(""), (winston::Id)track, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); }, false);
    case Tracks::Turnout3:
    case Tracks::Turnout4:
    case Tracks::Turnout5:
    case Tracks::Turnout6:
    case Tracks::Turnout7:
    case Tracks::Turnout8:
    case Tracks::Turnout9:
        return winston::Turnout::make(std::string(""), (winston::Id)track, [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State { return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction); }, true);
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
    /*
#define attachSignalY2020(track, SignalClass, guardedConnection) \
    track->attachSignal(SignalClass::make([=](const winston::Aspects aspect)->const winston::State { return this->callbacks.signalUpdateCallback(track, guardedConnection, aspect); }), guardedConnection);

    attachSignalY2020(g1, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g1, winston::SignalKS, winston::Track::Connection::B);
    attachSignalY2020(a, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(b, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g2, winston::SignalKS, winston::Track::Connection::A);
    attachSignalY2020(g3, winston::SignalKS, winston::Track::Connection::B);*/
}


Y2021Railway::Y2021Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2021RailwayTracks, Y2021RailwayRoutes, Y2021RailwaySections>(callbacks){};
const std::string Y2021Railway::name()
{
    return std::string("Y2021Railway");
}

Y2021Railway::AddressTranslator::AddressTranslator(Y2021Railway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Track::Shared Y2021Railway::AddressTranslator::turnout(const winston::Address address) const
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
    case 14: track = Tracks::DoubleSlipTurnout15_16; break;
    case 15: track = Tracks::DoubleSlipTurnout15_16; break;
    case 16: track = Tracks::Turnout17; break;
    case 17: track = Tracks::Turnout18; break;
    case 18: track = Tracks::Turnout19; break;
    case 19: track = Tracks::Turnout20; break;
    }
    return railway->track(track);
}

const winston::Address Y2021Railway::AddressTranslator::address(winston::Track& track) const
{
    auto t = track.shared_from_this();
    switch (railway->trackEnum(t))
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
    case Tracks::DoubleSlipTurnout15_16: return 14 /* & 15 */; break;
    case Tracks::Turnout17: return 16; break;
    case Tracks::Turnout18: return 17; break;
    case Tracks::Turnout19: return 18; break;
    case Tracks::Turnout20: return 19; break;
    }
    return 0;
}

winston::Route::Shared Y2021Railway::AddressTranslator::route(const winston::Address address) const
{
    return nullptr;
}

const winston::Address Y2021Railway::AddressTranslator::address(winston::Route::Shared track) const
{
    return 0;
}

winston::Track::Shared Y2021Railway::define(const Tracks track)
{
    using namespace winston::library::track;
    auto turnoutCallback = [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State
    {
        return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction);
    };
    auto doubleSlipTurnoutCallback = [this, track](winston::Track& turnout, const winston::DoubleSlipTurnout::Direction direction) -> winston::State
    {
        return this->callbacks.doubleSlipUpdateCallback(static_cast<winston::DoubleSlipTurnout&>(turnout), direction);
    };
    switch (track)
    {
        BUMPER(N1, 3 * Roco::G12 + Roco::G1);
        BUMPER(N2, Roco::R10);
        BUMPER(N3, 2 * Roco::G12 + Roco::G1);
        BUMPER(GBF1, 4 * Roco::G12 + Roco::R10);
        BUMPER(GBF2, Roco::R4 + Roco::G12);
        BUMPER(GBF3a, 2 * Roco::G1);
        BUMPER(GBF4a, Roco::G1);
        BUMPER(PBF1a, 3 * Roco::G1);
        RAIL(PBF_To_N, Roco::R2 + Roco::G12 + Roco::R3);
        RAIL(PBF1, Roco::G4 + 2 * Roco::R3);
        RAIL(PBF2, Roco::G4 + Roco::G12 + Roco::G14);
        RAIL(PBF2a, Roco::R3);
        RAIL(PBF3, 2 * Roco::G1);
        RAIL(PBF3a, Roco::G12 + Roco::G14 + Roco::G1);
        RAIL(GBF3b, Roco::G1);
        RAIL(GBF4b, 3 * Roco::G1);
        RAIL(B1, Roco::G12 + Roco::R2);
        RAIL(B2, Roco::G12 + Roco::G1 + Roco::G4 + Roco::R3);
        RAIL(B3, 3 * Roco::R3 + Roco::G14 + Roco::R2);
        RAIL(B4, Roco::G14 + 5 * Roco::R2);
        RAIL(B5, Roco::G12 + 3 * Roco::G1);
        RAIL(B6, Roco::G1 + 5 * Roco::R2 + Roco::G12);
        RAIL(B_PBF2_PBF1, Roco::DG1);
        RAIL(B_To_GBF, Roco::G12 + Roco::G14);
        RAIL(T7_To_T8, 2 * Roco::R2);
        TURNOUT(Turnout1, turnoutCallback, Roco::BW23, false);
        TURNOUT(Turnout3, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout9, turnoutCallback, Roco::BW23, false);
        TURNOUT(Turnout11, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout17, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout19, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout20, turnoutCallback, Roco::W15, false);
        DOUBLESLIPTURNOUT(DoubleSlipTurnout15_16, doubleSlipTurnoutCallback, Roco::DKW15);
        TURNOUT(Turnout2, turnoutCallback, Roco::BW23, true);
        TURNOUT(Turnout4, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout5, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout6, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout7, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout8, turnoutCallback, Roco::BW23, true);
        TURNOUT(Turnout10, turnoutCallback, Roco::BW23, true);
        TURNOUT(Turnout12, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout13, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout14, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout18, turnoutCallback, Roco::W15, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void Y2021Railway::connect()
{
#define LOCAL_TRACK(var)  auto var = this->track(Tracks::var);
#define LOCAL_TURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::Turnout>(this->track(Tracks::var));
#define LOCAL_DOUBLESLIPTURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::DoubleSlipTurnout>(this->track(Tracks::var));
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2a);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(PBF3);
    LOCAL_TRACK(PBF3a);
    LOCAL_TRACK(GBF1);
    LOCAL_TRACK(GBF2);
    LOCAL_TRACK(GBF3a);
    LOCAL_TRACK(GBF3b);
    LOCAL_TRACK(GBF4a);
    LOCAL_TRACK(GBF4b);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B_PBF2_PBF1);
    LOCAL_TRACK(B_To_GBF);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(PBF_To_N);
    LOCAL_TRACK(T7_To_T8);
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
    LOCAL_TRACK(DoubleSlipTurnout15_16);
    LOCAL_TRACK(Turnout17);
    LOCAL_TRACK(Turnout18);
    LOCAL_TRACK(Turnout19);
    LOCAL_TRACK(Turnout20);

    const auto A = winston::Track::Connection::A;
    const auto B = winston::Track::Connection::B;
    const auto C = winston::Track::Connection::C;
    const auto D = winston::Track::Connection::D;

    // outer loop
    Turnout1->connect(B, PBF2a, A)
        ->connect(B, Turnout4, A)
        ->connect(B, PBF2, A)
        ->connect(B, Turnout7, B)
        ->connect(A, T7_To_T8, A)
        ->connect(B, Turnout8, C)
        ->connect(A, B1, A)
        ->connect(B, Turnout9, A)
        ->connect(B, B2, A)
        ->connect(B, Turnout13, B)
        ->connect(A, B3, A)
        ->connect(B, Turnout1, A);

    // inner loop
    Turnout2->connect(A, Turnout3, A)
        ->connect(B, PBF3, A)
        ->connect(B, Turnout6, A)
        ->connect(B, PBF3a, A)
        ->connect(B, B4, A)
        ->connect(B, Turnout10, C)
        ->connect(A, Turnout11, A)
        ->connect(B, B5, A)
        ->connect(B, Turnout12, A)
        ->connect(B, B6, A)
        ->connect(B, Turnout2, C);

    // lower track
    PBF1a->connect(A, Turnout5, B)
        ->connect(A, PBF1, A)
        ->connect(B, Turnout8, B);

    // inner turnouts
    Turnout1->connect(C, Turnout2, B);
    Turnout4->connect(C, B_PBF2_PBF1, A)
        ->connect(B, Turnout5, C);
    Turnout6->connect(C, Turnout7, C);
    Turnout9->connect(C, Turnout10, B);
    Turnout11->connect(C, B_To_GBF, A)
        ->connect(B, DoubleSlipTurnout15_16, A);
    Turnout12->connect(C, Turnout13, C);
    Turnout19->connect(C, Turnout20, C);

    // nebengleise
    Turnout3->connect(C, PBF_To_N, A)
        ->connect(B, Turnout14, A)
        ->connect(B, N2, A);
    Turnout14->connect(C, N1, A);
    DoubleSlipTurnout15_16->connect(D, N3, A);

    // GBF
    DoubleSlipTurnout15_16->connect(C, Turnout17, A)
        ->connect(B, Turnout18, A)
        ->connect(B, GBF1, A);
    DoubleSlipTurnout15_16->connect(B, GBF4b, A)
        ->connect(B, Turnout20, B)
        ->connect(A, GBF4a, A);
    Turnout17->connect(C, GBF3b, A)
        ->connect(B, Turnout19, A)
        ->connect(B, GBF3a, A);
    Turnout18->connect(C, GBF2, A);
}

winston::Route::Shared Y2021Railway::define(const Routes route)
{
    
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2a);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(PBF3);
    LOCAL_TRACK(PBF3a);
    LOCAL_TRACK(GBF1);
    LOCAL_TRACK(GBF2);
    LOCAL_TRACK(GBF3a);
    LOCAL_TRACK(GBF3b);
    LOCAL_TRACK(GBF4a);
    LOCAL_TRACK(GBF4b);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B_PBF2_PBF1);
    LOCAL_TRACK(B_To_GBF);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(PBF_To_N);
    LOCAL_TRACK(T7_To_T8);
    LOCAL_TURNOUT(Turnout1);
    LOCAL_TURNOUT(Turnout2);
    LOCAL_TURNOUT(Turnout3);
    LOCAL_TURNOUT(Turnout4);
    LOCAL_TURNOUT(Turnout5);
    LOCAL_TURNOUT(Turnout6);
    LOCAL_TURNOUT(Turnout7);
    LOCAL_TURNOUT(Turnout8);
    LOCAL_TURNOUT(Turnout9);
    LOCAL_TURNOUT(Turnout10);
    LOCAL_TURNOUT(Turnout11);
    LOCAL_TURNOUT(Turnout12);
    LOCAL_TURNOUT(Turnout13);
    LOCAL_TURNOUT(Turnout14);
    LOCAL_DOUBLESLIPTURNOUT(DoubleSlipTurnout15_16);
    LOCAL_TURNOUT(Turnout17);
    LOCAL_TURNOUT(Turnout18);
    LOCAL_TURNOUT(Turnout19);
    LOCAL_TURNOUT(Turnout20);

#define ROUTE(id, ...)  case Routes::id: { return winston::Route::make((int)Routes::id, __VA_ARGS__); }
#define PATH(...)   winston::Route::Path{__VA_ARGS__}
#define PROTECTIONS(...)   winston::Route::Protections{__VA_ARGS__}
#define PATH_TRACK(x) winston::Route::Track::make(x)
#define PATH_TURNOUT(x, dir) winston::Route::Turnout::make(x, winston::Turnout::Direction::dir)

    switch (route)
    {
    ROUTE(B3_PBF1, 
        "B3 --> PBF1",
        PATH(
            PATH_TRACK(B3),
            PATH_TURNOUT(Turnout1, A_B),
            PATH_TRACK(PBF2a),
            PATH_TURNOUT(Turnout4, A_C),
            PATH_TRACK(B_PBF2_PBF1),
            PATH_TURNOUT(Turnout5, A_C),
            PATH_TRACK(PBF1)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout2, A_C)
        ))
    ROUTE(B3_PBF2,
        "B3 --> PBF2",
        PATH(
            PATH_TRACK(B3),
            PATH_TURNOUT(Turnout1, A_B),
            PATH_TRACK(PBF2a),
            PATH_TURNOUT(Turnout4, A_B),
            PATH_TRACK(PBF2)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout2, A_C),
            PATH_TURNOUT(Turnout5, A_B),
            PATH_TURNOUT(Turnout6, A_B)
        ))
    ROUTE(B3_PBF3,
        "B3 --> PBF3",
        PATH(
            PATH_TRACK(B3),
            PATH_TURNOUT(Turnout1, A_C),
            PATH_TURNOUT(Turnout2, A_B),
            PATH_TURNOUT(Turnout3, A_B),
            PATH_TRACK(PBF3),
            PATH_TURNOUT(Turnout6, A_B),
            PATH_TRACK(PBF3a)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout7, A_B)
        )) 

    ROUTE(B3_N1,
        "B3 --> N1",
        PATH(
            PATH_TRACK(B3),
            PATH_TURNOUT(Turnout1, A_C),
            PATH_TURNOUT(Turnout2, A_B),
            PATH_TURNOUT(Turnout3, A_C),
            PATH_TRACK(PBF_To_N),
            PATH_TURNOUT(Turnout14, A_C),
            PATH_TRACK(N1)
        ))
    ROUTE(B3_N2,
        "B3 --> N2",
        PATH(
            PATH_TRACK(B3),
            PATH_TURNOUT(Turnout1, A_C),
            PATH_TURNOUT(Turnout2, A_B),
            PATH_TURNOUT(Turnout3, A_C),
            PATH_TRACK(PBF_To_N),
            PATH_TURNOUT(Turnout14, A_B),
            PATH_TRACK(N2)
        ))
    ROUTE(B6_PBF3,
        "B6 --> PBF3",
        PATH(
            PATH_TRACK(B6),
            PATH_TURNOUT(Turnout2, A_C),
            PATH_TURNOUT(Turnout3, A_B),
            PATH_TRACK(PBF3),
            PATH_TURNOUT(Turnout6, A_B),
            PATH_TRACK(PBF3a)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout1, A_B)
        ))

    ROUTE(B6_N1,
        "B6 --> N1",
        PATH(
            PATH_TRACK(B6),
            PATH_TURNOUT(Turnout2, A_C),
            PATH_TURNOUT(Turnout3, A_C),
            PATH_TRACK(PBF_To_N),
            PATH_TURNOUT(Turnout14, A_C),
            PATH_TRACK(N1)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout1, A_B)
        ))
    ROUTE(B6_N2,
        "B6 --> N2",
        PATH(
            PATH_TRACK(B6),
            PATH_TURNOUT(Turnout2, A_C),
            PATH_TURNOUT(Turnout3, A_C),
            PATH_TRACK(PBF_To_N), 
            PATH_TURNOUT(Turnout14, A_B),
            PATH_TRACK(N2)
        ),
        PROTECTIONS(
            PATH_TURNOUT(Turnout1, A_B)
        ))
    default:
        winston::logger.warn("undefined route: ", route._to_string());
        break;
    }
    
    return nullptr;
}

Y2021Railway::Section::Shared Y2021Railway::define(const Y2021Railway::Sections section)
{
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2a);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(PBF3);
    LOCAL_TRACK(PBF3a);
    LOCAL_TRACK(GBF1);
    LOCAL_TRACK(GBF2);
    LOCAL_TRACK(GBF3a);
    LOCAL_TRACK(GBF3b);
    LOCAL_TRACK(GBF4a);
    LOCAL_TRACK(GBF4b);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B_PBF2_PBF1);
    LOCAL_TRACK(B_To_GBF);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(PBF_To_N);
    LOCAL_TRACK(T7_To_T8);
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
    LOCAL_TRACK(DoubleSlipTurnout15_16);
    LOCAL_TRACK(Turnout17);
    LOCAL_TRACK(Turnout18);
    LOCAL_TRACK(Turnout19);
    LOCAL_TRACK(Turnout20);

#define SECTION(id, type, ...)  case Y2021Railway::Sections::id: { return Y2021Railway::Section::make(Y2021Railway::Sections::id, type, winston::TrackSet(__VA_ARGS__)); }
#define FREE        winston::Section::Type::Free
#define TRANSIT     winston::Section::Type::Transit
#define SIDING      winston::Section::Type::Siding
#define PLATFORM    winston::Section::Type::Platform

    switch(section)
    {
        SECTION(PBF1a, SIDING, { PBF1a });
        SECTION(N1, SIDING, { N1 });
        SECTION(N2, SIDING, { N2 });
        SECTION(N3, SIDING, { N3 });
        SECTION(GBF1, SIDING, { GBF1 });
        SECTION(GBF2, SIDING, { GBF2 });
        SECTION(GBF3, SIDING, { GBF3a, Turnout19, GBF3b });
        SECTION(GBF4, SIDING, { GBF4a, Turnout20, GBF4b });

        SECTION(N, TRANSIT, { PBF_To_N, Turnout14 });
        SECTION(PBF12, TRANSIT, { Turnout1, PBF2a, Turnout4, B_PBF2_PBF1 });
        SECTION(GBF, TRANSIT, { B_To_GBF, DoubleSlipTurnout15_16, Turnout17, Turnout18 });

        SECTION(B1, FREE, { Turnout8, B1 });
        SECTION(B2, FREE, { Turnout9, B2, Turnout13});
        SECTION(B3, FREE, { B3 });
        SECTION(B4, FREE, { B4 });
        SECTION(B5, FREE, { Turnout10, Turnout11, B5, Turnout12 });
        SECTION(B6, FREE, { B6 });

        SECTION(PBF1, PLATFORM, { Turnout5, PBF1 });
        SECTION(PBF2, PLATFORM, { PBF2, Turnout7, T7_To_T8 });
        SECTION(PBF3, PLATFORM, { Turnout2, Turnout3, PBF3, Turnout6, PBF3a });
    default:
        winston::logger.warn("undefined section: ", section._to_string());
        break;
}
    return nullptr;
#undef SECTION
}

void Y2021Railway::attachDetectors()
{
}

const bool Y2021Railway::supportSections() const
{
    return true;
}

const bool Y2021Railway::supportRoutes() const
{
    return true;
}

const winston::Result Y2021Railway::validateFinal()
{
    auto result = this->validateFinalTracks();
    if (result != winston::Result::OK)
        return result;

    return this->validateFinalRoutes();
}
#endif

Y2024Railway::Y2024Railway(const Callbacks callbacks) : winston::RailwayWithRails<Y2024RailwayTracks, Y2024RailwayRoutes, Y2024RailwaySections, Y2024RailwaySegments>(callbacks) {};
const std::string Y2024Railway::name()
{
    return std::string("Y2024Railway");
}

Y2024Railway::AddressTranslator::AddressTranslator(Y2024Railway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Track::Shared Y2024Railway::AddressTranslator::turnout(const winston::Address address) const
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
    case 7: track = Tracks::DoubleSlipTurnout8_9; break;
    case 8: track = Tracks::DoubleSlipTurnout8_9; break;
    case 9: track = Tracks::Turnout10; break;
    case 10: track = Tracks::Turnout11; break;
    case 11: track = Tracks::Turnout12; break;
    }
    return railway->track(track);
}

const winston::Address Y2024Railway::AddressTranslator::address(winston::Track& track) const
{
    auto t = track.shared_from_this();
    switch (railway->trackEnum(t))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    case Tracks::Turnout3: return 2; break;
    case Tracks::Turnout4: return 3; break;
    case Tracks::Turnout5: return 4; break;
    case Tracks::Turnout6: return 5; break;
    case Tracks::Turnout7: return 6; break;
    case Tracks::DoubleSlipTurnout8_9: return 7; break;
    case Tracks::Turnout10: return 9; break;
    case Tracks::Turnout11: return 10; break;
    case Tracks::Turnout12: return 11; break;
    }
    return 0;
}

winston::Route::Shared Y2024Railway::AddressTranslator::route(const winston::Address address) const
{
    return nullptr;
}

const winston::Address Y2024Railway::AddressTranslator::address(winston::Route::Shared track) const
{
    return 0;
}

winston::Track::Shared Y2024Railway::define(const Tracks track)
{
    using namespace winston::library::track;
    auto turnoutCallback = [this, track](winston::Track& turnout, const winston::Turnout::Direction direction) -> winston::State
        {
            return this->callbacks.turnoutUpdateCallback(static_cast<winston::Turnout&>(turnout), direction);
        };
    auto doubleSlipTurnoutCallback = [this, track](winston::Track& turnout, const winston::DoubleSlipTurnout::Direction direction) -> winston::State
        {
            return this->callbacks.doubleSlipUpdateCallback(static_cast<winston::DoubleSlipTurnout&>(turnout), direction);
        };
    switch (track)
    {
        BUMPER(N1, 3*Roco::R2_14 + Roco::G1 + Roco::G12 + Roco::G14);
        BUMPER(N2, Roco::R10);
        BUMPER(N3, 2 * Roco::G1 + Roco::G12);
        BUMPER(N4, Roco::R2 + 2 * Roco::G1 + Roco::G12 + Roco::G14);
        BUMPER(N5, Roco::G1 + Roco::G14);
        BUMPER(PBF1a, Roco::G1 + Roco::G14);
        BUMPER(LS1, Roco::R10 + Roco::G1);
        BUMPER(LS2, Roco::G1 + Roco::G1);
        RAIL(PBF1, Roco::R10 + 2 * Roco::G1 + Roco::G12);
        RAIL(PBF2, Roco::G4 + Roco::G12);
        RAIL(B1, 3 * Roco::R2);
        RAIL(B2, 3 * Roco::R2);
        RAIL(B3, 3 * Roco::G1 + Roco::G14);
        RAIL(B4, Roco::R10 + Roco::G1 + Roco::G14);
        RAIL(B5, Roco::G14);
        RAIL(B6, 3 * Roco::R2);
        RAIL(B7, 3 * Roco::R2);
        RAIL(Z1, Roco::R3 + Roco::D5);
        RAIL(Z2, Roco::R2_14);
        RAIL(Z3, Roco::R2);
        RAIL(Z4, Roco::R2 + Roco::D8);
        RAIL(Z5, Roco::R2);
        RAIL(Z6, Roco::Roco::G14);
        TURNOUT(Turnout1, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout2, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout3, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout4, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout5, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout6, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout7, turnoutCallback, Roco::W15, false);
        DOUBLESLIPTURNOUT(DoubleSlipTurnout8_9, doubleSlipTurnoutCallback, Roco::DKW15);
        TURNOUT(Turnout10, turnoutCallback, Roco::W15, true);
        TURNOUT(Turnout11, turnoutCallback, Roco::W15, false);
        TURNOUT(Turnout12, turnoutCallback, Roco::W15, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void Y2024Railway::connect()
{
#define LOCAL_TRACK(var)  auto var = this->track(Tracks::var);
#define LOCAL_TURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::Turnout>(this->track(Tracks::var));
#define LOCAL_DOUBLESLIPTURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::DoubleSlipTurnout>(this->track(Tracks::var));
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B7);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(N4);
    LOCAL_TRACK(N5);
    LOCAL_TRACK(LS1);
    LOCAL_TRACK(LS2);
    LOCAL_TRACK(Z1);
    LOCAL_TRACK(Z2);
    LOCAL_TRACK(Z3);
    LOCAL_TRACK(Z4);
    LOCAL_TRACK(Z5);
    LOCAL_TRACK(Z6);
    LOCAL_TRACK(Turnout1);
    LOCAL_TRACK(Turnout2);
    LOCAL_TRACK(Turnout3);
    LOCAL_TRACK(Turnout4);
    LOCAL_TRACK(Turnout5);
    LOCAL_TRACK(Turnout6);
    LOCAL_TRACK(Turnout7);
    LOCAL_TRACK(DoubleSlipTurnout8_9);
    LOCAL_TRACK(Turnout10);
    LOCAL_TRACK(Turnout11);
    LOCAL_TRACK(Turnout12);

    const auto A = winston::Track::Connection::A;
    const auto B = winston::Track::Connection::B;
    const auto C = winston::Track::Connection::C;
    const auto D = winston::Track::Connection::D;

    // loop
    Turnout1->connect(B, PBF2, A)
        ->connect(B, Turnout3, B)
        ->connect(A, B1, A)
        ->connect(B, B2, A)
        ->connect(B, Turnout4, A)
        ->connect(B, B3, A)
        ->connect(B, Turnout6, B)
        ->connect(A, B5, A)
        ->connect(B, Turnout7, B)
        ->connect(A, B6, A)
        ->connect(B, B7, A)
        ->connect(B, Turnout1, A);

    // PBF
    Turnout1->connect(C, PBF1, A)
        ->connect(B, Turnout2, A)
        ->connect(B, PBF1a, A);
    Turnout2->connect(C, Turnout3, C);

    // Ausweichgleis
    Turnout4->connect(C, Turnout5, C);
    Turnout6->connect(C, B4, B)
        ->connect(A, Turnout5, A)
        ->connect(B, N5, A);

    // Z
    Turnout7->connect(C, Z1, A)
        ->connect(B, DoubleSlipTurnout8_9, D)
        ->connect(C, Z3, A)
        ->connect(B, Turnout10, A)
        ->connect(B, N3, A);
    Turnout10->connect(C, N2, A);

    DoubleSlipTurnout8_9->connect(B, Z4, A)
        ->connect(B, Turnout11, C)
        ->connect(A, Z6, A)
        ->connect(B, N4, A);

    DoubleSlipTurnout8_9->connect(A, Z2, A)
        ->connect(B, N1, A);

    Turnout11->connect(B, Z5, A)
        ->connect(B, Turnout12, A)
        ->connect(B, LS2, A);
    Turnout12->connect(C, LS1, A);
}

winston::Route::Shared Y2024Railway::define(const Routes route)
{
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B7);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(N4);
    LOCAL_TRACK(N5);
    LOCAL_TRACK(LS1);
    LOCAL_TRACK(LS2);
    LOCAL_TRACK(Z1);
    LOCAL_TRACK(Z2);
    LOCAL_TRACK(Z3);
    LOCAL_TRACK(Z4);
    LOCAL_TRACK(Z5);
    LOCAL_TRACK(Z6);
    LOCAL_TURNOUT(Turnout1);
    LOCAL_TURNOUT(Turnout2);
    LOCAL_TURNOUT(Turnout3);
    LOCAL_TURNOUT(Turnout4);
    LOCAL_TURNOUT(Turnout5);
    LOCAL_TURNOUT(Turnout6);
    LOCAL_TURNOUT(Turnout7);
    LOCAL_DOUBLESLIPTURNOUT(DoubleSlipTurnout8_9);
    LOCAL_TURNOUT(Turnout10);
    LOCAL_TURNOUT(Turnout11);
    LOCAL_TURNOUT(Turnout12);

#define ROUTE(id, ...)  case Routes::id: { return winston::Route::make((int)Routes::id, __VA_ARGS__); }
#define PATH(...)   winston::Route::Path{__VA_ARGS__}
#define PROTECTIONS(...)   winston::Route::Protections{__VA_ARGS__}
#define PATH_TRACK(x) winston::Route::Track::make(x)
#define PATH_TURNOUT(x, dir) winston::Route::Turnout::make(x, winston::Turnout::Direction::dir)

    switch (route)
    {
        ROUTE(B7_PBF1,
            "B7 --> PBF1",
            PATH(
                PATH_TRACK(B7),
                PATH_TURNOUT(Turnout1, A_C),
                PATH_TRACK(PBF1)
            ),
            PROTECTIONS(
                PATH_TURNOUT(Turnout2, A_B),
                PATH_TURNOUT(Turnout3, A_B)
            ))
        ROUTE(B7_PBF2,
            "B7 --> PBF2",
            PATH(
                PATH_TRACK(B7),
                PATH_TURNOUT(Turnout1, A_B),
                PATH_TRACK(PBF2)
            ))
        ROUTE(B1_PBF1,
            "B1 --> PBF1",
            PATH(
                PATH_TRACK(B1),
                PATH_TURNOUT(Turnout3, A_C),
                PATH_TURNOUT(Turnout2, A_C),
                PATH_TRACK(PBF1)
            ))
        ROUTE(B1_PBF2,
            "B1 --> PBF2",
            PATH(
                PATH_TRACK(B1),
                PATH_TURNOUT(Turnout3, A_B),
                PATH_TRACK(PBF2)
            ),
            PROTECTIONS(
                PATH_TURNOUT(Turnout2, A_B)
            ))
    default:
        winston::logger.warn("undefined route: ", route._to_string());
        break;
    }

    return nullptr;
}

Y2024Railway::Section::Shared Y2024Railway::define(const Y2024Railway::Sections section)
{
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B7);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(N4);
    LOCAL_TRACK(N5);
    LOCAL_TRACK(LS1);
    LOCAL_TRACK(LS2);
    LOCAL_TRACK(Z1);
    LOCAL_TRACK(Z2);
    LOCAL_TRACK(Z3);
    LOCAL_TRACK(Z4);
    LOCAL_TRACK(Z5);
    LOCAL_TRACK(Z6);
    LOCAL_TRACK(Turnout1);
    LOCAL_TRACK(Turnout2);
    LOCAL_TRACK(Turnout3);
    LOCAL_TRACK(Turnout4);
    LOCAL_TRACK(Turnout5);
    LOCAL_TRACK(Turnout6);
    LOCAL_TRACK(Turnout7);
    LOCAL_TRACK(DoubleSlipTurnout8_9);
    LOCAL_TRACK(Turnout10);
    LOCAL_TRACK(Turnout11);
    LOCAL_TRACK(Turnout12);

#define SECTION(id, type, ...)  case Y2024Railway::Sections::id: { return Y2024Railway::Section::make(Y2024Railway::Sections::id, type, winston::TrackSet(__VA_ARGS__)); }
#define FREE        winston::Section::Type::Free
#define TRANSIT     winston::Section::Type::Transit
#define SIDING      winston::Section::Type::Siding
#define PLATFORM    winston::Section::Type::Platform

    switch (section)
    {
        SECTION(PBF1a, SIDING, { PBF1a });
        SECTION(N1, SIDING, { N1 });
        SECTION(N2, SIDING, { N2 });
        SECTION(N3, SIDING, { N3 });
        SECTION(N4, SIDING, { N4 });
        SECTION(N5, SIDING, { N5 });

        SECTION(LS1, SIDING, { LS1 });
        SECTION(LS2, SIDING, { LS2 });
        SECTION(Z, SIDING, { Z1, Z2, Z3, Z4, Z5, Z6, DoubleSlipTurnout8_9, Turnout10, Turnout11, Turnout12 });

        SECTION(B1, FREE, { Turnout3, B1 });
        SECTION(B2, FREE, { B2, Turnout4 });
        SECTION(B3, FREE, { B3 });
        SECTION(B4, FREE, { B4, Turnout5 });
        SECTION(B5, FREE, { B5, Turnout6 });
        SECTION(B6, FREE, { B6, Turnout7 });
        SECTION(B7, FREE, { B7, Turnout1 });

        SECTION(PBF1, PLATFORM, { PBF1, Turnout2 });
        SECTION(PBF2, PLATFORM, { PBF2 });
    default:
        winston::logger.warn("undefined section: ", section._to_string());
        break;
    }
    return nullptr;
#undef SECTION
}

Y2024Railway::Segment::Shared Y2024Railway::define(const Segments segment)
{
    LOCAL_TRACK(PBF1a);
    LOCAL_TRACK(PBF1);
    LOCAL_TRACK(PBF2);
    LOCAL_TRACK(B1);
    LOCAL_TRACK(B2);
    LOCAL_TRACK(B3);
    LOCAL_TRACK(B4);
    LOCAL_TRACK(B5);
    LOCAL_TRACK(B6);
    LOCAL_TRACK(B7);
    LOCAL_TRACK(N1);
    LOCAL_TRACK(N2);
    LOCAL_TRACK(N3);
    LOCAL_TRACK(N4);
    LOCAL_TRACK(N5);
    LOCAL_TRACK(LS1);
    LOCAL_TRACK(LS2);
    LOCAL_TRACK(Z1);
    LOCAL_TRACK(Z2);
    LOCAL_TRACK(Z3);
    LOCAL_TRACK(Z4);
    LOCAL_TRACK(Z5);
    LOCAL_TRACK(Z6);
    LOCAL_TRACK(Turnout1);
    LOCAL_TRACK(Turnout2);
    LOCAL_TRACK(Turnout3);
    LOCAL_TRACK(Turnout4);
    LOCAL_TRACK(Turnout5);
    LOCAL_TRACK(Turnout6);
    LOCAL_TRACK(Turnout7);
    LOCAL_TRACK(DoubleSlipTurnout8_9);
    LOCAL_TRACK(Turnout10);
    LOCAL_TRACK(Turnout11);
    LOCAL_TRACK(Turnout12);

#define SEGMENT(id, ...)  case id: { return Y2024Railway::Segment::make(id, winston::TrackSet(__VA_ARGS__)); }

    switch (segment)
    {
        SEGMENT(0, { B6, Turnout7 });
        SEGMENT(1, { N1 });
        SEGMENT(2, { LS2 });
        SEGMENT(3, { LS1 });
        SEGMENT(4, { B7, Turnout1 });
        SEGMENT(5, { B2, Turnout4, B3, Turnout5, B5 });
        SEGMENT(6, { Z1, DoubleSlipTurnout8_9, Z2, Z3, Z4, Z5, Z6, Turnout10, Turnout11, Turnout12});
        SEGMENT(7, { PBF1, Turnout2 });
        SEGMENT(8, { PBF2 });
        SEGMENT(9, { N2 });
        SEGMENT(10, { N3 });
        SEGMENT(11, { N4 });
        SEGMENT(12, { PBF1a });
        SEGMENT(13, { Turnout3, B1 });
        SEGMENT(14, { Turnout5, B4 });
        SEGMENT(15, { N5 });
    default:
        winston::logger.warn("undefined segment: ", segment);
        break;
    }
    return nullptr;
}

void Y2024Railway::attachDetectors()
{
}

const bool Y2024Railway::supportSections() const
{
    return true;
}

const bool Y2024Railway::supportRoutes() const
{
    return true;
}

const bool Y2024Railway::supportSegments() const
{
    return true;
}

const winston::Result Y2024Railway::validateFinal()
{
    auto result = this->validateFinalTracks();
    if (result != winston::Result::OK)
        return result;

    return this->validateFinalRoutes();
}