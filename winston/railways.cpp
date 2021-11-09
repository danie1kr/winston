#include "railways.h"
#include <stdexcept>
#include <string>
#include "magic_enum.hpp"

#define BUMPER(track, ...) case Tracks::track: return winston::Bumper::make(std::string(magic_enum::enum_name(Tracks::track)), __VA_ARGS__); 
#define RAIL(track, ...) case Tracks::track: return winston::Rail::make(std::string(magic_enum::enum_name(Tracks::track)), __VA_ARGS__); 
#define TURNOUT(track, callback, ...) case Tracks::track: return winston::Turnout::make(std::string(magic_enum::enum_name(Tracks::track)), callback, __VA_ARGS__); 


MiniRailway::MiniRailway(const Callbacks callbacks) : winston::RailwayWithRails<MiniRailwayTracks>(callbacks) {};

winston::Track::Shared MiniRailway::define(const Tracks track)
{
    switch (track) {
        BUMPER(A);
        BUMPER(B);
        BUMPER(C);
    case Tracks::Turnout1:
        return winston::Turnout::make(std::string("Turnout1"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void MiniRailway::connect(std::array<winston::Track::Shared, tracksCount()>& tracks)
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
    TURNOUT(Turnout1, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    TURNOUT(Turnout2, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    TURNOUT(Turnout3, [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); });
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void SignalTestRailway::connect(std::array<winston::Track::Shared, tracksCount()>& tracks)
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
RailwayWithSiding::AddressTranslator::AddressTranslator(RailwayWithSiding::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared RailwayWithSiding::AddressTranslator::turnout(const winston::Address address)
{
    Tracks track;
    switch (address)
    {
    default:
    case 0: track = Tracks::Turnout1; break;
    case 1: track = Tracks::Turnout2; break;
    }
    return std::dynamic_pointer_cast<winston::Turnout>(railway->track(track));
}

const winston::Address RailwayWithSiding::AddressTranslator::address(winston::Track::Shared track)
{
    switch (railway->trackEnum(track))
    {
    default:
    case Tracks::Turnout1: return 0; break;
    case Tracks::Turnout2: return 1; break;
    }
    return 0;
}

winston::Locomotive::Shared RailwayWithSiding::AddressTranslator::locomotive(const winston::Address address)
{
    return nullptr;
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
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, track == Tracks::Turnout2);
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void RailwayWithSiding::connect(std::array<winston::Track::Shared, tracksCount()> & tracks)
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
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout2:
    case Tracks::Turnout5:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }
}

void TimeSaverRailway::connect(std::array<winston::Track::Shared, tracksCount()>& tracks)
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

Y2020Railway::AddressTranslator::AddressTranslator(Y2020Railway::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared Y2020Railway::AddressTranslator::turnout(const winston::Address address)
{
	Tracks track;
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
    return std::dynamic_pointer_cast<winston::Turnout>(railway->track(track));
}

winston::Locomotive::Shared Y2020Railway::AddressTranslator::locomotive(const winston::Address address)
{
    return nullptr;
}

const winston::Address Y2020Railway::AddressTranslator::address(winston::Track::Shared track)
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
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout3:
    case Tracks::Turnout4:
    case Tracks::Turnout5:
    case Tracks::Turnout6:
    case Tracks::Turnout7:
    case Tracks::Turnout8:
    case Tracks::Turnout9:
        return winston::Turnout::make(std::string(""), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }

}

void Y2020Railway::connect(std::array<winston::Track::Shared, tracksCount()>& tracks)
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

SignalRailway::SignalRailway(const Callbacks callbacks) : winston::RailwayWithRails<SignalRailwayTracks>(callbacks) {};
const std::string SignalRailway::name()
{
    return std::string("SignalRailway");
}

SignalRailway::AddressTranslator::AddressTranslator(SignalRailway::Shared railway) : winston::DigitalCentralStation::AddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Turnout::Shared SignalRailway::AddressTranslator::turnout(const winston::Address address)
{
    Tracks track;
    switch (address)
    {
    default:
    case 0: track = Tracks::Turnout1; break;
    case 1: track = Tracks::Turnout2; break;
    case 2: track = Tracks::Turnout3; break;
    case 3: track = Tracks::Turnout4; break;
    case 4: track = Tracks::Turnout5; break;
    }
    return std::dynamic_pointer_cast<winston::Turnout>(railway->track(track));
}

winston::Locomotive::Shared SignalRailway::AddressTranslator::locomotive(const winston::Address address)
{
    return nullptr;
}

const winston::Address SignalRailway::AddressTranslator::address(winston::Track::Shared track)
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
    case Tracks::Turnout1: return winston::Turnout::make(std::string("Turnout1"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout2: return winston::Turnout::make(std::string("Turnout2"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, false);
    case Tracks::Turnout3: return winston::Turnout::make(std::string("Turnout3"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout4: return winston::Turnout::make(std::string("Turnout4"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    case Tracks::Turnout5: return winston::Turnout::make(std::string("Turnout5"), [this, track](winston::Track::Shared turnout, const winston::Turnout::Direction direction) -> winston::State { winston::Turnout::Shared s = std::dynamic_pointer_cast<winston::Turnout, winston::Track>(turnout); return this->callbacks.turnoutUpdateCallback(s, direction); }, true);
    default:
        winston::hal::fatal(std::string("track ") + std::string(magic_enum::enum_name(track)) + std::string("not in switch"));
        return winston::Bumper::make();
    }

}

void SignalRailway::connect(std::array<winston::Track::Shared, tracksCount()>& tracks)
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

    a->connect(winston::Track::Connection::A, KS(0), t2, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, g1, winston::Track::Connection::B, KS(0))
        ->connect(winston::Track::Connection::A, KS(0), t4, winston::Track::Connection::C)
        ->connect(winston::Track::Connection::A, g4, winston::Track::Connection::B, KS(0))
        ->connect(winston::Track::Connection::A, g5, winston::Track::Connection::B)
        ->connect(winston::Track::Connection::A, KS(0), t5, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g6, winston::Track::Connection::B, KS(0))
        ->connect(winston::Track::Connection::A, g7, winston::Track::Connection::B, KS(0))
        ->connect(winston::Track::Connection::A, t1, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::C, t2, winston::Track::Connection::C);

    t1->connect(winston::Track::Connection::B, g2, winston::Track::Connection::A, KS(0))
        ->connect(winston::Track::Connection::B, t3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, g3, winston::Track::Connection::A)
        ->connect(winston::Track::Connection::B, KS(0), t4, winston::Track::Connection::B);

    t3->connect(winston::Track::Connection::C, b, winston::Track::Connection::A, KS(0));

    t5->connect(winston::Track::Connection::C, c, winston::Track::Connection::A, KS(0));

#define attachSignalSR(track, SignalClass, guardedConnection) \
    track->attachSignal(SignalClass::make([=](const winston::Signal::Aspects aspect)->const winston::State { return this->callbacks.signalUpdateCallback(track, guardedConnection, aspect); }), guardedConnection);

    //attachSignal(g1, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g1, winston::SignalKS, winston::Track::Connection::B);
    //attachSignalSR(a, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(b, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(c, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g2, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g3, winston::SignalKS, winston::Track::Connection::B);
    //attachSignal(g4, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g4, winston::SignalKS, winston::Track::Connection::B);
    //attachSignalSR(g5, winston::SignalKS, winston::Track::Connection::A);
    //attachSignal(g5, winston::SignalKS, winston::Track::Connection::B);
    //attachSignal(g6, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g6, winston::SignalKS, winston::Track::Connection::B);
    //attachSignal(g7, winston::SignalKS, winston::Track::Connection::A);
    //attachSignalSR(g7, winston::SignalKS, winston::Track::Connection::B);
}