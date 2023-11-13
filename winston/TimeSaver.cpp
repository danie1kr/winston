
#include "TimeSaver.h"

#define BUMPER(track, ...) case Tracks::_enumerated::track: return winston::Bumper::make(#track, __VA_ARGS__); 
#define RAIL(track, ...) case Tracks::_enumerated::track: return winston::Rail::make(#track, __VA_ARGS__); 
#define TURNOUT(track, callback, ...) case Tracks::_enumerated::track: return winston::Turnout::make(#track, callback, __VA_ARGS__); 
#define DOUBLESLIPTURNOUT(track, callback, ...) case Tracks::_enumerated::track: return winston::DoubleSlipTurnout::make(#track, callback, __VA_ARGS__); 

TimeSaverRailway::TimeSaverRailway(const Callbacks callbacks) : winston::RailwayWithRails<TimeSaverRailwayTracks>(callbacks) {};
const std::string TimeSaverRailway::name()
{
    return std::string("TimeSaverRailway");
}

TimeSaverRailway::AddressTranslator::AddressTranslator(TimeSaverRailway::Shared railway) : winston::DigitalCentralStation::TurnoutAddressTranslator(), Shared_Ptr<AddressTranslator>(), railway(railway) { };

winston::Track::Shared TimeSaverRailway::AddressTranslator::turnout(const winston::Address address) const
{
    Tracks track = Tracks::T1;
    switch (address)
    {
    case 0: track = Tracks::T1; break;
    case 1: track = Tracks::T2; break;
    case 2: track = Tracks::T3; break;
    case 3: track = Tracks::T4; break;
    case 4: track = Tracks::T5; break;
    case 5: track = Tracks::T6; break;
    default:
        winston::logger.warn(std::string("track ") + std::string(track._to_string()) + std::string(" not in switch"));
    }
    return railway->track(track);
}

const winston::Address TimeSaverRailway::AddressTranslator::address(winston::Track::Shared track) const
{
    switch (railway->trackEnum(track))
    {
    case Tracks::T1: return 0; break;
    case Tracks::T2: return 1; break;
    case Tracks::T3: return 2; break;
    case Tracks::T4: return 3; break;
    case Tracks::T5: return 4; break;
    case Tracks::T6: return 5; break;
    default:
        winston::logger.warn(std::string("track ") + track->name()+ std::string(" not in switch"));
    }
    return 0;
}

winston::Route::Shared TimeSaverRailway::AddressTranslator::route(const winston::Address address) const
{
    return nullptr;
}

const winston::Address TimeSaverRailway::AddressTranslator::address(winston::Route::Shared track) const
{
    return 0;
}

winston::Track::Shared TimeSaverRailway::define(const Tracks track)
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
        BUMPER(A, 2 * Roco::G1 + Roco::G12);
        BUMPER(B, 1 * Roco::G1 + Roco::G12);
        BUMPER(C, 1 * Roco::G1 + Roco::G12);
        BUMPER(E, 1 * Roco::G1 + Roco::G12);
        BUMPER(F, 2 * Roco::G1 + Roco::G12);
        BUMPER(G, 2 * Roco::G1 + Roco::G12);
        RAIL(D, Roco::G1);
        TURNOUT(T1, turnoutCallback, Roco::W15, true);
        TURNOUT(T2, turnoutCallback, Roco::W15, false);
        TURNOUT(T3, turnoutCallback, Roco::W15, true);
        TURNOUT(T4, turnoutCallback, Roco::W15, false);
        TURNOUT(T5, turnoutCallback, Roco::W15, false);
        TURNOUT(T6, turnoutCallback, Roco::W15, false);
    default:
        winston::hal::fatal(std::string("track ") + std::string(track._to_string()) + std::string( "not in switch"));
        return winston::Bumper::make();
    }
}

void TimeSaverRailway::connect()
{
#define LOCAL_TRACK(var)  auto var = this->track(Tracks::var);
#define LOCAL_TURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::Turnout>(this->track(Tracks::var));
#define LOCAL_DOUBLESLIPTURNOUT(var)  auto var = std::dynamic_pointer_cast<winston::DoubleSlipTurnout>(this->track(Tracks::var));
    LOCAL_TRACK(A);
    LOCAL_TRACK(B);
    LOCAL_TRACK(C);
    LOCAL_TRACK(D);
    LOCAL_TRACK(E);
    LOCAL_TRACK(F);
    LOCAL_TRACK(G);
    LOCAL_TRACK(T1);
    LOCAL_TRACK(T2);
    LOCAL_TRACK(T3);
    LOCAL_TRACK(T4);
    LOCAL_TRACK(T5);
    LOCAL_TRACK(T6);

    const auto a = winston::Track::Connection::A;
    const auto b = winston::Track::Connection::B;
    const auto c = winston::Track::Connection::C;
    const auto d = winston::Track::Connection::D;

    // upper
    A->connect(a, T1, b)
        ->connect(a, T2, a)
        ->connect(b, B, a);

    // middle
    C->connect(a, T3, a)
        ->connect(b, T4, a)
        ->connect(b, D, a)
        ->connect(b, T5, b)
        ->connect(a, E, a);

    // lower
    F->connect(a, T6, b)
        ->connect(a, G, a);

    // upper to middle
    T3->connect(c, T1, c);
    T2->connect(c, T5, c);

    // middle to lower
    T4->connect(c, T6, c);
}

winston::Route::Shared TimeSaverRailway::define(const Routes route)
{
    LOCAL_TRACK(A);
    LOCAL_TRACK(B);
    LOCAL_TRACK(C);
    LOCAL_TRACK(D);
    LOCAL_TRACK(E);
    LOCAL_TRACK(F);
    LOCAL_TRACK(G);
    LOCAL_TURNOUT(T1);
    LOCAL_TURNOUT(T2);
    LOCAL_TURNOUT(T3);
    LOCAL_TURNOUT(T4);
    LOCAL_TURNOUT(T5);
    LOCAL_TURNOUT(T6);
    const auto a = winston::Track::Connection::A;
    const auto b = winston::Track::Connection::B;
    const auto c = winston::Track::Connection::C;
    const auto d = winston::Track::Connection::D;

#define ROUTE(id, ...)  case Routes::id: { return winston::Route::make((int)Routes::id, __VA_ARGS__); }
#define PATH(...)   winston::Route::Path{__VA_ARGS__}
#define PROTECTIONS(...)   winston::Route::Protections{__VA_ARGS__}
#define PATH_TRACK(x) winston::Route::Track::make(x)
#define PATH_TURNOUT(x, dir) winston::Route::Turnout::make(x, winston::Turnout::Direction::dir)

    /*
        A_B, B_A,
        A_E, E_A,
        C_B, B_C,
        C_E, E_C,
        C_G, G_C,
        F_G, G_F
    */
    switch (route)
    {
        ROUTE(A_B,
            "A --> B",
            PATH(
                PATH_TRACK(A),
                PATH_TURNOUT(T1, A_B),
                PATH_TURNOUT(T2, A_B),
                PATH_TRACK(B)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T3, A_B),
                PATH_TURNOUT(T5, A_B)
            ))
        ROUTE(B_A,
            "B --> A",
            PATH(
                PATH_TRACK(B),
                PATH_TURNOUT(T2, A_B),
                PATH_TURNOUT(T1, A_B),
                PATH_TRACK(A)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T3, A_B),
                PATH_TURNOUT(T5, A_B)
            ))
        ROUTE(A_E,
            "A --> E",
            PATH(
                PATH_TRACK(A),
                PATH_TURNOUT(T1, A_B),
                PATH_TURNOUT(T2, A_C),
                PATH_TURNOUT(T5, A_C),
                PATH_TRACK(E)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T3, A_B)
            ))
        ROUTE(E_A,
            "E --> A",
            PATH(
                PATH_TRACK(E),
                PATH_TURNOUT(T5, A_C),
                PATH_TURNOUT(T2, A_C),
                PATH_TURNOUT(T1, A_B),
                PATH_TRACK(A)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T3, A_B)
            ))
        ROUTE(C_B,
            "C --> B",
            PATH(
                PATH_TRACK(C),
                PATH_TURNOUT(T3, A_C),
                PATH_TURNOUT(T1, A_C),
                PATH_TURNOUT(T2, A_B),
                PATH_TRACK(B)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T4, A_B)
            ))
        ROUTE(B_C,
            "B --> C",
            PATH(
                PATH_TRACK(B),
                PATH_TURNOUT(T2, A_B),
                PATH_TURNOUT(T1, A_C),
                PATH_TURNOUT(T3, A_C),
                PATH_TRACK(C)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T4, A_B)
            ))
        ROUTE(C_E,
            "C --> E",
            PATH(
                PATH_TRACK(C),
                PATH_TURNOUT(T3, A_B),
                PATH_TURNOUT(T4, A_B),
                PATH_TURNOUT(T5, A_B),
                PATH_TRACK(E)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T1, A_B),
                PATH_TURNOUT(T2, A_B),
                PATH_TURNOUT(T6, A_B)
            ))
        ROUTE(E_C,
            "E --> C",
            PATH(
                PATH_TRACK(E),
                PATH_TURNOUT(T5, A_B),
                PATH_TURNOUT(T4, A_B),
                PATH_TURNOUT(T3, A_B),
                PATH_TRACK(C)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T1, A_B),
                PATH_TURNOUT(T2, A_B),
                PATH_TURNOUT(T6, A_B)
            ))

        ROUTE(C_G,
            "C --> G",
            PATH(
                PATH_TRACK(C),
                PATH_TURNOUT(T3, A_B),
                PATH_TURNOUT(T4, A_C),
                PATH_TURNOUT(T6, A_C),
                PATH_TRACK(G)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T1, A_B)
            ))
        ROUTE(G_C,
            "G --> C",
            PATH(
                PATH_TRACK(G),
                PATH_TURNOUT(T6, A_C),
                PATH_TURNOUT(T4, A_C),
                PATH_TURNOUT(T3, A_B),
                PATH_TRACK(C)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T1, A_B)
            ))
        ROUTE(F_G,
            "F --> G",
            PATH(
                PATH_TRACK(F),
                PATH_TURNOUT(T6, A_B),
                PATH_TRACK(G)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T4, A_B)
            ))
        ROUTE(G_F,
            "G --> F",
            PATH(
                PATH_TRACK(G),
                PATH_TURNOUT(T6, A_B),
                PATH_TRACK(F)
            ),
            PROTECTIONS(
                PATH_TURNOUT(T4, A_B)
            ))
    default:
        winston::logger.warn("unsupported route: ", route._to_string());
        break;
    }

    return nullptr;
}

void TimeSaverRailway::attachDetectors()
{
}
const winston::Result TimeSaverRailway::init()
{
    auto result = this->initRails();
    if (result != winston::Result::OK)
    {
        winston::logger.err("TimeSaverRailway initRails failed with ", result);
        return result;
    }
    result = this->initRoutes();
    if (result != winston::Result::OK)
    {
        winston::logger.err("TimeSaverRailway initRoutes failed with ", result);
        return result;
    }

    return winston::Result::OK;
}

const winston::Result TimeSaverRailway::validateFinal()
{
    auto result = winston::RailwayWithRails<TimeSaverRailwayTracks>::validateFinal();
    if (result != winston::Result::OK)
        return result;

    return winston::RailwayAddonRoutes<TimeSaverRailwayRoutes>::validateFinal();
}