﻿#ifdef WINSTON_PLATFORM_TEENSY
#define Binary_h
#undef B1
// keeps binary_h from beeing used which kills our 
#endif

#include "Kornweinheim.h"

#ifdef WINSTON_PLATFORM_WIN_x64
#define FLASHMEM
#endif

#ifdef WINSTON_PACKED_WEBFILES
#include "resources.h"
#endif 

/*
    TODO: 
        - retry commands if no answer came
        - show state via signals
        - initial connect, wait until first answer
        - set maximum light value when setting up signal

    ROADMAP:
        - DCC-Detector
            - send loco position to frontend
        - frontend
            - place loco on track, have it simulate detectors when entering/leaving sections
            - section signals

    PCB v2:
        - new tiny signals
        - new signal connectors
            correct pin numbering
            solder pads for cables
        - dsub-connector
            - 5 - signal SPI + 12V+GND
            - 2 - DCC
            - 7 x 1 per detector section
            - 2 - 5V+GND
            - 4 - Serial TX/RX

*/

winston::DigitalCentralStation::Callbacks Kornweinheim::z21Callbacks()
{
    // default Callbacks already apply
    winston::DigitalCentralStation::Callbacks callbacks;

    callbacks.locomotiveUpdateCallback = [=](winston::Locomotive& loco, bool busy, bool forward, unsigned char speed, uint32_t functions)
    {
        loco.update(busy, forward, speed, functions);

#ifdef WINSTON_WITH_WEBSOCKET
        this->webUI.locoSend(loco);
#endif    
    };

    callbacks.connectedCallback = [=]() {
        this->digitalCentralStationConnected();
    };

    callbacks.specialAccessoryProcessingCallback = [=](const uint16_t address, const uint8_t state) -> const bool {
        
        if(address == 500)
        {
            this->digitalCentralStationConnected();
        }
        else if (address >= 400 && address < 400 + this->railway->routesCount())
        {
            if (auto route = this->railway->route(address - 400))
                this->orderRouteSet(route, true);
        }
        else
        {
            return true;
        }
        
        return false;
    };
	callbacks.systemStatusCallback = [=](const bool shortCircuit, const uint16_t temperature, const uint16_t trackAmp, const uint16_t trackVoltage) {
		statusDisplay.digitalStationStatus(shortCircuit, temperature, trackAmp, trackVoltage);
		};

    return callbacks;
}

winston::Locomotive::Callbacks Kornweinheim::locoCallbacks()
{
    winston::Locomotive::Callbacks callbacks;

    callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) -> const winston::Result {
        this->signalTower->order(winston::Command::make([this, address, speed, forward](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
        return winston::Result::OK;
    };

    callbacks.functions = [=](const winston::Address address, const uint32_t functions) -> const winston::Result  {
        this->signalTower->order(winston::Command::make([this, address, functions](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoFunction(address, functions);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
        return winston::Result::OK;
    };

    callbacks.signalPassed = [=](const winston::Locomotive::Const loco, const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result
    {
        LOG_INFO(loco->name(), " passed ", track->name(), " ", connection);

		if (track->index == Tracks::B6 && connection == winston::Track::Connection::B)
		{
			LOG_WARN("Loco ", loco->name(), " passed B6|B");
		}

        auto signal = track->signalGuarding(connection);
        signalTower->setSignalsForLocoPassing(track, connection, pass);
        return winston::Result::OK;
    };

    callbacks.stopped = [=](auto loco) {
        this->locomotiveShed.store(loco);
        };

    return callbacks;
}

winston::Railway::Callbacks Kornweinheim::railwayCallbacks()
{
    winston::Railway::Callbacks callbacks;

    callbacks.turnoutUpdateCallback = [=](winston::Turnout& turnout, const winston::Turnout::Direction direction) -> const winston::State
    {    
        // tell the signal box to update the signals
        this->signalTower->setSignalsFor(turnout);

        // check if there is a locomotive on this segment. If so, invalidate its speed trap as we will have issues calculating the correct length since entry
        for (auto& loco : this->locomotiveShed.shed())
            if (loco->segment() == turnout.segment())
                loco->invalidateSpeedTrap();
        
        // stop here if we are still initializing and learn the initial states of the turnouts
        if (!this->isReady())
            return winston::State::Finished;

        // once only: set physical turnout to this location to align it with what the dcs thinks it is
        auto track = turnout.shared_from_this();
        if (direction != winston::Turnout::Direction::Changing && !checkedTurnoutsDuringInit[track])
        {
            this->orderTurnoutToggle(turnout, direction);
            checkedTurnoutsDuringInit[track] = true;
        }

#ifdef WINSTON_WITH_WEBSOCKET
        // tell the ui what happens
        this->webUI.turnoutSendState(turnout.name(), direction, turnout.locked());
#endif
        for(auto route : this->routesInProgress)
        {
            auto state = route->reportTurnout(turnout, direction);
            if (state == winston::Route::State::Set)
            {
                this->routesInProgress.erase(std::remove(this->routesInProgress.begin(), this->routesInProgress.end(), route), this->routesInProgress.end());
#ifdef WINSTON_WITH_WEBSOCKET
                this->webUI.routeState(*route);
#endif
            }
        }

#ifdef WINSTON_ENABLE_TURNOUT_GROUPS
        if (direction != winston::Turnout::Direction::Changing)
        {
            auto groupedTurnouts = this->railway->turnoutsSharingGroupWith(turnout);
            groupedTurnouts.erase(turnout.shared_from_this());

            for (auto other : groupedTurnouts)
            {
                if (other->direction() != winston::Turnout::Direction::Changing)
                {
                    for (const auto group : other->groups())
                    {
                        if (group.second == winston::Turnout::GroupDirection::Unknown)
                            continue;
                        if (turnout.isInGroup(group.first))
                        {
                            const auto dir = group.second == winston::Turnout::GroupDirection::Same ? direction : winston::Turnout::otherDirection(direction);
                            if (other->direction() != dir)
                            {
                                this->signalTower->order(winston::Command::make([other, dir, this](const winston::TimePoint& created) -> const winston::State
                                    {
                                        if (winston::hal::now() - created > WINSTON_TURNOUT_GROUP_TOGGLE)
                                        {
                                            if (other->direction() != dir)
                                                this->orderTurnoutToggle(*other, dir);
                                            return winston::State::Finished;
                                        }
                                        return winston::State::Running;
                                    }, __PRETTY_FUNCTION__));
                            }
                        }
                    }
                }
            }
        }
#endif

        LOG_INFO("Turnout ", turnout.name(), " set to direction ", winston::Turnout::DirectionToString(direction));

        return winston::State::Finished;
    };

    callbacks.doubleSlipUpdateCallback = [=](winston::DoubleSlipTurnout&  turnout, const winston::DoubleSlipTurnout::Direction direction) -> const winston::State
    {
        // tell the signal box to update the signals
        this->signalTower->setSignalsFor(turnout);

        // stop here if we are still initializing and learn the initial states of the turnouts
        if (!this->isReady())
            return winston::State::Finished;
        
        // once only: set physical turnout to this location to align it with what the dcs thinks it is
        auto track = turnout.shared_from_this();
        if (direction != winston::DoubleSlipTurnout::Direction::Changing && !checkedTurnoutsDuringInit[track])
        {
            this->orderDoubleSlipTurnoutToggle(turnout, direction);
            checkedTurnoutsDuringInit[track] = true;
        }

#ifdef WINSTON_WITH_WEBSOCKET
        // tell the ui what happens
        this->webUI.doubleSlipSendState(turnout.name(), direction, turnout.locked());
#endif
        for (auto route : this->routesInProgress)
        {
            auto state = route->reportTurnout(turnout, direction);
            if (state == winston::Route::State::Set)
            {
                this->routesInProgress.erase(std::remove(this->routesInProgress.begin(), this->routesInProgress.end(), route), this->routesInProgress.end());
#ifdef WINSTON_WITH_WEBSOCKET
                this->webUI.routeState(*route);
#endif
            }
        }

        LOG_INFO("Turnout ", turnout.name(), " set to direction ", winston::DoubleSlipTurnout::DirectionToString(direction));

        return winston::State::Finished;
    };

    /*callbacks.dccDetectorCallback = [=](winston::Detector::Shared, const winston::Address address) -> winston::Result
    {
        // not used here
        return winston::Result::OK;
    };*/

    return callbacks;
}

#ifdef WINSTON_WITH_WEBSOCKET
    const winston::Result Kornweinheim::on_http(WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource) {
    const std::string path_confirmation_yes("/confirm_yes");
    const std::string path_confirmation_maybe("/confirm_maybe");
    const std::string path_confirmation_no("/confirm_no");
    /*
    if (resource.compare(path_confirmation_yes) == 0)
    {
        this->userConfirmation->confirm(winston::ConfirmationProvider::Answer::Yes);
    }
    else if (resource.compare(path_confirmation_maybe) == 0)
    {
        this->userConfirmation->confirm(winston::ConfirmationProvider::Answer::Maybe);
    }
    else if (resource.compare(path_confirmation_no) == 0)
    {
        this->userConfirmation->confirm(winston::ConfirmationProvider::Answer::No);
    }
    else*/
    {
#ifdef WINSTON_PACKED_WEBFILES
        const auto file = resource.substr(0, resource.find("?"));
        auto resourceFile = resources::get(file);
        if (resourceFile != nullptr)
        {
            connection.status(200);
            connection.header("content-type"_s, resourceFile->content_type);// +"; charset = UTF - 8"_s);
#ifdef WINSTON_PLATFORM_WIN_x64
            std::string payload;
            payload.append((char*)resourceFile->data, resourceFile->size);
            connection.body(payload);
#else
            connection.body(resourceFile->data, resourceFile->size, 64);
#endif
        }
        else
#endif
            return winston::Result::NotFound;
    }

    return winston::Result::OK;
}

#endif

// setup our model railway system
void Kornweinheim::systemSetup() {
    // the user defined railway and its address translator
    TEENSY_CRASHLOG_BREADCRUMB(2, 0x101);
    this->railway = RAILWAY_CLASS::make(railwayCallbacks());

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x102);
    // z21
    this->z21Socket = UDPSocket::make(z21IP, z21Port);

    this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

    // the internal signal box
    this->signalTower = winston::SignalTower::make(this->locomotiveShed);

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x103);
    // the system specific digital central station
    auto at = std::static_pointer_cast<winston::DigitalCentralStation::TurnoutAddressTranslator>(addressTranslator);
    auto udp = std::static_pointer_cast<winston::hal::Socket>(this->z21Socket);
    this->digitalCentralStation = Z21::make(udp, at, *this, this->signalTower, z21Callbacks());

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    // a debug injector
    auto dcs = std::static_pointer_cast<winston::DigitalCentralStation>(this->digitalCentralStation);
    this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(dcs);
#endif

    // signals
    TEENSY_CRASHLOG_BREADCRUMB(2, 0x104);
#ifdef WINSTON_PLATFORM_TEENSY
    this->signalInterfaceDevices.push_back(SignalInterfaceDevice::make(33, TLC5947::SPI_Clock));
    this->signalInterfaceDevices.push_back(SignalInterfaceDevice::make(34, TLC5947::SPI_Clock));
    auto TLC5947Off = Arduino_GPIOOutputPin::make(30, Arduino_GPIOOutputPin::State::High);
#else
#pragma message("using one SignalInterfaceDevice for two devices")
    this->signalInterfaceDevices.push_back(SignalInterfaceDevice::make(3, TLC5947::SPI_Clock));
    this->signalInterfaceDevices.push_back(SignalInterfaceDevice::make(4, TLC5947::SPI_Clock));
    auto TLC5947Off = this->signalInterfaceDevices[0]->getOutputPinDevice(4);
#endif
    /*
    // two chains, each having two TLC5947 devices
    const unsigned int chainedTLC5947s = 3;
    unsigned int signalDeviceId = 0;
    for (const auto& device : this->signalInterfaceDevices)
    {
        device->init();
        device->skipSend(true);
        this->signalDevices.push_back(TLC5947::make(signalDeviceId++, chainedTLC5947s * 24, device, TLC5947Off));
    }
    */

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x105);
    unsigned int signalDeviceId = 0;
    for (const auto& device : this->signalInterfaceDevices)
    {
        device->init();
        device->skipSend(true);
    }
    // two chains, first having two TLC5947 devices
    this->signalDevices.push_back(TLC5947::make(signalDeviceId++, 2 * 24, this->signalInterfaceDevices[0], TLC5947Off));
    // two chains, second having TLC5947
    this->signalDevices.push_back(TLC5947::make(signalDeviceId++, 1 * 24, this->signalInterfaceDevices[1], TLC5947Off));

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x106);
    this->signalController = winston::SignalController::make(0, this->signalDevices);

    // storage
    TEENSY_CRASHLOG_BREADCRUMB(2, 0x107);
    this->storageLayout = Storage::make(std::string(this->name()).append(".").append("winston.storage"), 256 * 1024);
    if (this->storageLayout->init() != winston::Result::OK)
        LOG_ERROR("Kornweinheim.init: Storage Layout Init failed");
    this->storageMicroLayout = Storage::make(std::string(this->name()).append(".").append("winston.micro.storage"), 256 * 1024);
    if (this->storageMicroLayout->init() != winston::Result::OK)
        LOG_ERROR("Kornweinheim.init: Storage Micro Layout Init failed");
    this->storageLocoShed = Storage::make("winston.locoshed.storage", 32 * 1024); // 1.5k per loco
    if (this->storageLocoShed->init() != winston::Result::OK)
        LOG_ERROR("Kornweinheim.init: Storage LocoShed failed");

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x108);
    this->populateSheds();

    // detectors
#ifdef WINSTON_PLATFORM_TEENSY

#elif defined(WINSTON_PLATFORM_WIN_x64)
    this->serial = SerialDeviceWin::make();
    this->serial->init(5);
#endif
    TEENSY_CRASHLOG_BREADCRUMB(2, 0x109);
    this->routesInProgress.clear();

    TEENSY_CRASHLOG_BREADCRUMB(2, 0x110);
#ifdef WINSTON_HAL_USE_WEBSERVER
    this->setupWebServer(this->storageLayout, this->storageMicroLayout, this->addressTranslator, 8080
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
		,[&] (const bool inject) -> const winston::Result { return this->debugInjectorToggle(inject); }
#endif
    );
    \
    winston::logger.setCallback([this](const winston::Logger::Entry& entry) {
		statusDisplay.log(entry);
        this->webUI.log(entry);
    });
#endif
};

void Kornweinheim::createSignals(winston::SignalController& signalController, RAILWAY_CLASS::Shared railway, winston::Railway::Callbacks::SignalUpdateCallback signalUpdateCallback)
{
    auto signalUpdateAlwaysHalt = [=](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
        {
            return winston::State::Finished;
        };

#ifdef WINSTON_KLEINWEINHEIM
    // Pbf
    signalController.attach<winston::SignalKS>(railway->track(Tracks::PBF1), winston::Track::Connection::A, 24.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::PBF1), winston::Track::Connection::B, 0.f, signalUpdateCallback);

    signalController.attach<winston::SignalKS>(railway->track(Tracks::PBF2), winston::Track::Connection::A, 64.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::PBF2), winston::Track::Connection::B, 60.f, signalUpdateCallback);

    signalController.attach<winston::SignalH>(railway->track(Tracks::PBF1a), winston::Track::Connection::A, 32.f, signalUpdateCallback);

    // N + LS
    signalController.attach<winston::SignalH>(railway->track(Tracks::N1), winston::Track::Connection::A, 24.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::N2), winston::Track::Connection::A, 24.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::N3), winston::Track::Connection::A, 64.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::N4), winston::Track::Connection::A, 0.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::N5), winston::Track::Connection::A, 24.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::LS1), winston::Track::Connection::A, 55.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::LS2), winston::Track::Connection::A, 55.f, signalUpdateCallback);

    // Track
    signalController.attach<winston::SignalH>(railway->track(Tracks::B1), winston::Track::Connection::A, winston::library::track::Roco::R2, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B1), winston::Track::Connection::B, winston::library::track::Roco::R2 / 2.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B2), winston::Track::Connection::A, winston::library::track::Roco::R2 / 2.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B2), winston::Track::Connection::B, winston::library::track::Roco::R2 / 2.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B3), winston::Track::Connection::A, 24, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B3), winston::Track::Connection::B, 24, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B4), winston::Track::Connection::A, 24, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B4), winston::Track::Connection::B, 24, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B6), winston::Track::Connection::A, 0, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B6), winston::Track::Connection::B, winston::library::track::Roco::R2 / 2.f, signalUpdateCallback);
    signalController.attach<winston::SignalKS>(railway->track(Tracks::B7), winston::Track::Connection::A, winston::library::track::Roco::R2 / 2.f, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::B7), winston::Track::Connection::B, winston::library::track::Roco::R2, signalUpdateCallback);

    // leaving inner tracks
    signalController.attach<winston::SignalKS>(railway->track(Tracks::Z1), winston::Track::Connection::A, winston::library::track::Roco::R2 / 2, signalUpdateCallback);
    signalController.attach<winston::SignalH>(railway->track(Tracks::Z3), winston::Track::Connection::A, 35.f, signalUpdateCallback);

    // track end bumper signals
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::N1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::N2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::N3), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::N4), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::N5), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::LS1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::LS2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    signalController.attach<winston::SignalAlwaysHalt>(railway->track(Tracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
#else
// PBF
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF3a), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF3), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::A, 5U, signalUpdateCallback);

// rechte Strecke
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B1), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B4), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::B1), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::B4), winston::Track::Connection::A, 5U, signalUpdateCallback);

// obere Strecke
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B5), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B5), winston::Track::Connection::A, 5U, signalUpdateCallback);

// linke Strecke
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::B3), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::B6), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B3), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::B6), winston::Track::Connection::A, 5U, signalUpdateCallback);

// Abstellgleise
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::PBF_To_N), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::N1), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::N2), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::N3), winston::Track::Connection::A, 5U, signalUpdateCallback);

// GBF
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::GBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::GBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::GBF4a), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::GBF4b), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::GBF4b), winston::Track::Connection::B, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::GBF3a), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalKS>(this->railway->track(Tracks::GBF3b), winston::Track::Connection::A, 5U, signalUpdateCallback);
this->signalController.attach<winston::SignalH>(this->railway->track(Tracks::GBF3b), winston::Track::Connection::B, 5U, signalUpdateCallback);

// don't care for ports here
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N3), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF3a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
this->signalController.attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF4a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
#endif
}

void Kornweinheim::setupSignals()
{
    auto signalUpdateCallback = [=](winston::Track& track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
#ifdef WINSTON_WITH_WEBSOCKET
        // send to web socket server
        this->webUI.signalSendState(track.name(), connection, aspects);
#endif

        // update physical light
        this->signalController->update(*track.signalGuarding(connection));
        LOG_INFO("Signal at ", track.name(), "|", winston::Track::ConnectionToString(connection), " set to ", winston::Signal::buildAspects(aspects));

        return winston::State::Finished;
    };


    this->createSignals(*this->signalController.get(), this->railway, signalUpdateCallback);
    
    /*
#ifdef WINSTON_KLEINWEINHEIM
    // Pbf
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::B, 5U, signalUpdateCallback);

    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::B, 5U, signalUpdateCallback);

    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::A, 5U, signalUpdateCallback);
    
    // N + LS
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N3), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N4), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N5), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::LS1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::LS2), winston::Track::Connection::A, 5U, signalUpdateCallback);

    // Track
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B1), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B3), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B3), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B4), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B4), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B6), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B6), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B7), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B7), winston::Track::Connection::B, 5U, signalUpdateCallback);

    // leaving inner tracks
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::Z1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::Z3), winston::Track::Connection::A, 5U, signalUpdateCallback);

    unsigned int dncPort = 999;
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N3), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N4), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N5), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::LS1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::LS2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);

#else
    // PBF
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF3a), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF3), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::A, 5U, signalUpdateCallback);

    // rechte Strecke
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B1), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B4), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B4), winston::Track::Connection::A, 5U, signalUpdateCallback);

    // obere Strecke
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B5), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B5), winston::Track::Connection::A, 5U, signalUpdateCallback);
    
    // linke Strecke
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B3), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::B6), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B3), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::B6), winston::Track::Connection::A, 5U, signalUpdateCallback);
    
    // Abstellgleise
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::PBF_To_N), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::N3), winston::Track::Connection::A, 5U, signalUpdateCallback);

    // GBF
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::GBF1), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::GBF2), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::GBF4a), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::GBF4b), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::GBF4b), winston::Track::Connection::B, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::GBF3a), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalKS>(this->railway->track(Tracks::GBF3b), winston::Track::Connection::A, 5U, signalUpdateCallback);
    this->signalController->attach<winston::SignalH>(this->railway->track(Tracks::GBF3b), winston::Track::Connection::B, 5U, signalUpdateCallback);

    // don't care for ports here
    unsigned int dncPort = 999;
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::N3), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF1), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF2), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF3a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalController->attach<winston::SignalAlwaysHalt>(this->railway->track(Tracks::GBF4a), winston::Track::Connection::DeadEnd, 5U, signalUpdateAlwaysHalt);
#endif*/
}

void Kornweinheim::systemLoop()
{
    {
        TEENSY_CRASHLOG_BREADCRUMB(5, 0x1);
#ifdef WINSTON_WITH_WEBSOCKET
#ifdef WINSTON_STATISTICS
        winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "webServer");
#endif
        this->webUI.step();
#endif
    }

    {
        TEENSY_CRASHLOG_BREADCRUMB(5, 0x2);
        static auto lastPosUpdatePrint = winston::hal::now();
        for (auto& loco : this->locomotiveShed.shed())
        {
            if (loco->isRailed()
#ifdef WINSTON_DETECTOR_ADDRESS
                && loco->address() == WINSTON_DETECTOR_ADDRESS
#endif
                )
            {
                TEENSY_CRASHLOG_BREADCRUMB(5, 0x200);
                loco->update();
#ifndef WINSTON_PLATFORM_TEENSY
                if (winston::hal::now() > lastPosUpdatePrint + toMilliseconds(133))
                 {
                    TEENSY_CRASHLOG_BREADCRUMB(5, 0x201);
                    auto pos = loco->position();
                    TEENSY_CRASHLOG_BREADCRUMB(5, 0x202);
                    LOG_INFO("loco ", loco->name(), " on ", pos.trackName(), "-", pos.connection(), "+", pos.distance());
                    lastPosUpdatePrint = winston::hal::now();
                    TEENSY_CRASHLOG_BREADCRUMB(5, 0x203);
                }
#endif
            }
        }
    }

#ifdef WINSTON_LOCO_STATUS_INTERVAL
    {
		TEENSY_CRASHLOG_BREADCRUMB(5, 0x3);
        if(winston::hal::now() > lastLocoStatusRequest + WINSTON_LOCO_STATUS_INTERVAL)
        {
            for (auto& loco : this->locomotiveShed.shed())
            {
                if (loco->isRailed())
                {
                    this->signalTower->order(winston::Command::make([&](const winston::TimePoint& created) -> const winston::State
                        {
						    this->digitalCentralStation->getLocoInfo(loco->address());
                            return winston::State::Finished;
                        }, __PRETTY_FUNCTION__));
                }
            }
            lastLocoStatusRequest = winston::hal::now();
        }
    }
#endif
#ifndef WINSTON_PLATFORM_TEENSY
#ifdef WINSTON_LOCO_POSITION_WEBUI_INTERVAL
    {
#ifndef WINSTON_REALWORLD
        if (enableDetectorInject) {
#endif
            if (winston::hal::now() > lastLocoWebUIPositionUpdate + WINSTON_LOCO_POSITION_WEBUI_INTERVAL)
            {
                TEENSY_CRASHLOG_BREADCRUMB(5, 0x4);
                this->webUI.locoPositionsSend();
                lastLocoWebUIPositionUpdate = winston::hal::now();
            }
#ifndef WINSTON_REALWORLD
        }
#endif
    }
#endif
#endif

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    {
        debugInjectorLoop();
    }
#endif

#if defined(WINSTON_PLATFORM_WIN_x64) && defined(WINSTON_WITH_STATUSDISPLAY)
    {
        statusDisplay.tick();
    }
#endif
}

winston::Result Kornweinheim::detectorUpdate(winston::Detector::Shared detector, winston::Locomotive& loco)
{
    // theoretical delays:
    /*
    	speed					
	km/h	    25	     60	     80	    120	    160
	m/s	         6,94	 16,67	 22,22	 33,33	 44,44
	m/s @ 1:87	 0,08	  0,19	  0,26	  0,38	  0,51
	mm/s	    79,82	191,57	255,43	383,14	510,86
d	1	         0,08	  0,19	  0,26	  0,38	  0,51
e	2	         0,16	  0,38	  0,51	  0,77	  1,02
l	4	         0,32	  0,77	  1,02	  1,53	  2,04
a	8	         0,64	  1,53	  2,04	  3,07	  4,09
y	16	         1,28	  3,07	  4,09	  6,13	  8,17
ms	32	         2,55	  6,13	  8,17	 12,26	 16,35
    speed in mm/s * delay in ms / 1000 = overshoot

    breaking distance = v * v / 2 * a

    */
    auto now = winston::hal::now(); // - offset (=2 ms Card->IC, +1ms IC->Teensy, +5ms Teensy->Teensy == 8ms <=> 2.04mm @ scaled 0.255 m/s @ 22.2m/s = 80 km/h
    // actual work as we do not want to section the receiver
    this->signalTower->order(winston::Command::make([detector, loco, now](const winston::TimePoint& created) -> const winston::State
        {
            // loco is now at
            //auto current = detector->position();
           // auto deviation = loco.drive() - current;


            return winston::State::Finished;
        }, __PRETTY_FUNCTION__));
    /*
        loco = fromNFCAddress(address)
        is now at
        at actual = track:connection - distance

        // update calculated distance and report error
        auto mismatch = updateLocoPos(loco) - acutal

        updateLocoPos(loco)
        {
            auto previous = loco.position();
            Duration timeOnTour = 0;
            auto pos = loco.drive(timeOnTour);   // updates loco position

            this->signalTower->order(winston::Command::make([=](const winston::TimePoint &created) -> const winston::State
            {
                railway -> updateSectionOccupancy(previous, pos, timeOnTour, loco.trainLength() = 100);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));

            this->signalTower->order(winston::Command::make([signalTower](const winston::TimePoint &created) -> const winston::State
            {
                signalbox -> updateSignalsAccordingToSections();
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
        }

        loco.drive(Duration &timeOnTour)
        {
            timeOnTour = inMilliseconds(this->lastUpdate - winston::hal::now());
            this->pos = this->pos + this->physicalSpeed() * timeOnTour;
            return this->pos;
        }
    */
    return winston::Result::OK;
}

const winston::Result Kornweinheim::setupDetectors()
{
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    this->loDiSocket = LoDi::createLoDiDebugSocket();
#else
    this->loDiSocket = TCPSocket::make(loDiIP, loDiPort);
#endif
    this->loDi = LoDi::make(this->loDiSocket);
    this->detectorDevice = this->loDi;

    this->loDi->discover();
    
    unsigned int count = 0;
    while (count++ < 24)
    {
        this->loDi->loop();
        winston::hal::delay(50);
    }

    this->loDiCommander = this->loDi->createS88Commander();

    winston::DetectorDevice::PortSegmentMap portSegmentMap;
    for (auto const& [id, segment] : this->railway->segments())
        portSegmentMap.insert({ id, segment });
    
    winston::DetectorDevice::Callbacks callbacks;
    callbacks.change = 
        [](const std::string detectorName, const winston::Locomotive::Shared loco, const bool forward, winston::Segment::Shared segment, const winston::Detector::Change change, const winston::TimePoint when) -> const winston::Result
        { 
#ifdef WINSTON_DETECTOR_ADDRESS
			if (loco->address() != WINSTON_DETECTOR_ADDRESS)
				return winston::Result::OK;
#endif
            LOG_INFO(loco->name(), change == winston::Detector::Change::Entered ? " entered " : " left ", segment->id);
            if (change == winston::Detector::Change::Entered)
                loco->entered(segment, when);
            else
                loco->left(segment, when);
            return winston::Result::OK; 
        };
    callbacks.occupied = 
        [](const std::string detectorName, winston::Segment::Shared segment, const winston::Detector::Change change, const winston::TimePoint when) -> const winston::Result
        {
            LOG_INFO("something ", change == winston::Detector::Change::Entered ? " entered " : " left ", segment->id);
            return winston::Result::OK;
        };
    callbacks.locoFromAddress = 
        [&](const winston::Address address) -> winston::Locomotive::Shared 
        {
            return this->locoFromAddress(address);
        };

    this->loDiCommander->init(portSegmentMap, callbacks);

    count = 0;
    while (!loDiCommander->isReady() && count++ < 24)
    {
        this->loDi->loop();
        winston::hal::delay(50);
    }

    if(!loDiCommander->isReady())
		LOG_ERROR("LoDi Commander not connected.");

    return winston::Result::OK;
}

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
const winston::Result Kornweinheim::debugInjectorToggle(const bool inject)
{
	this->enableDetectorInject = inject;
    auto loco = this->locoFromAddress(8);
    if (this->enableDetectorInject)
    {
		const auto position = winston::Position(this->railway->track(Tracks::PBF2), winston::Track::Connection::A, 100);
        this->orderTurnoutToggle(static_cast<winston::Turnout&>(*this->railway->track(Tracks::Turnout1)), winston::Turnout::Direction::A_B);
        this->orderTurnoutToggle(static_cast<winston::Turnout&>(*this->railway->track(Tracks::Turnout2)), winston::Turnout::Direction::A_B);
        this->orderTurnoutToggle(static_cast<winston::Turnout&>(*this->railway->track(Tracks::Turnout4)), winston::Turnout::Direction::A_B);
        this->orderTurnoutToggle(static_cast<winston::Turnout&>(*this->railway->track(Tracks::Turnout6)), winston::Turnout::Direction::A_B);
        this->orderTurnoutToggle(static_cast<winston::Turnout&>(*this->railway->track(Tracks::Turnout7)), winston::Turnout::Direction::A_B);
        loco->railOnto(position);
        loco->update(true, true, 30, 0);
    }
    else
    {
        loco->railOff();
    }

    return winston::Result::OK;
}

const winston::Result Kornweinheim::debugInjectorLoop()
{
    if (this->enableDetectorInject && winston::hal::now() > lastDetectorInject + WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
    {
        auto socket = std::dynamic_pointer_cast<winston::hal::DebugSocket>(this->loDiSocket);

        auto loco = this->locoFromAddress(8);
        auto position = loco->position();

        const auto previousSegment = position.track()->segment();

        const auto distance = loco->speed() * WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY / 1000ms;
        const auto transit = position.drive(distance, true, [](const winston::Track::Const track, const winston::Track::Connection connection, const winston::Signal::Pass pass) -> const winston::Result { return winston::Result::OK; });
        const auto segment = position.track()->segment();

		if (transit == winston::Position::Transit::CrossTrack && previousSegment != position.track()->segment())
		{
            {
                const auto packet = LoDi::railcomEvent(segment, loco->address(), winston::Detector::Change::Entered);
                socket->addRecvPacket(packet);
            } 
            {
                const auto packet = LoDi::railcomEvent(previousSegment, loco->address(), winston::Detector::Change::Left);
                socket->addRecvPacket(packet);
            }
		}
        lastDetectorInject = winston::hal::now();
    }
    return winston::Result::OK;
}
#endif

void Kornweinheim::systemSetupComplete()
{
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    this->stationDebugInjector->injectConnected();

    this->railway->turnouts([=](const Tracks track, winston::Turnout& turnout) {
        this->stationDebugInjector->injectTurnoutUpdate(turnout, std::rand() % 2 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C);
        });
    this->railway->doubleSlipTurnouts([=](const Tracks track, winston::DoubleSlipTurnout& turnout) {
        auto v = std::rand() % 4;
        auto dir = winston::DoubleSlipTurnout::Direction::A_B;
        if (v == 1) dir = winston::DoubleSlipTurnout::Direction::A_C;
        else if (v == 2) dir = winston::DoubleSlipTurnout::Direction::B_D;
        else if (v == 3) dir = winston::DoubleSlipTurnout::Direction::C_D;
        this->stationDebugInjector->injectDoubleSlipTurnoutUpdate(turnout, dir);
        });
 #endif

    this->inventStorylines();
    for (const auto& device : this->signalInterfaceDevices)
        device->skipSend(false);
    this->signalController->flush();
    this->signalTower->order(this->signalController->flushCommand());
}

void Kornweinheim::populateSheds()
{
    this->locomotiveShed.init(this->storageLocoShed);

    auto callbacks = locoCallbacks();
    winston::Position pos(this->railway->track(Tracks::N1), winston::Track::Connection::A, 100);

    winston::Locomotive::Functions standardFunctions = { {0, "Light"} };
    winston::Locomotive::Functions functionsBR114 = { {1, "R&uuml;cklicht"}, {2, "Scheinwerfer"}, {3, "Rangiergang"} };
    functionsBR114.insert(functionsBR114.begin(), standardFunctions.begin(), standardFunctions.end());

    winston::Locomotive::Functions functionsGravita = {
        { 1, "Sound"},
        { 2, "Signalhorn lang"},
        { 3, "Rotes Licht"}, 
        { 4, "Kuppeln vorn"}, 
        { 5, "Kuppeln hinten"},
        { 6, "L&uuml;fter"},
        { 7, "F&uuml;hrerstandbeleuchtung"},
        { 8, "Rangiergang"},
        { 9, "Fernlicht"},
        { 10, "Warnsignal"},
        { 11, "Spezielles Signal"},
        { 12, "Signalhorn kurz"},
        { 13, "Funkspruch #1"},
        { 14, "Funkspruch #2"},
        { 15, "Pressluft ablassen"},
        { 16, "Kurzpfiff"},
        { 17, "Kurvenquietschen"},
        { 18, "Fahrerstandst&uuml;r"},
        { 19, "Funkspruch #3"},
        { 20, "Funkspruch #4"},
        { 21, "Schienenst&ouml;&szlig;e"},
        { 22, "Sanden"}
    };
    functionsGravita.insert(functionsGravita.begin(), standardFunctions.begin(), standardFunctions.end());

    this->addLocomotive(callbacks, 3, functionsBR114, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 114", 164.f, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting);
    //this->addLocomotive(callbacks, 4, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 106", 150, (unsigned char)winston::Locomotive::Type::Shunting | (unsigned char)winston::Locomotive::Type::Goods);
    //this->addLocomotive(callbacks, 5, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 64", 150, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 6, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "E 11", 150.f, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 8, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 218", 180.f, (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 7, functionsGravita, pos, winston::Locomotive::defaultThrottleSpeedMap, "Gravita", 195.f, (unsigned char)winston::Locomotive::Type::Shunting | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 9, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 335", 90.f, (unsigned char)winston::Locomotive::Type::Shunting);

    auto DR = winston::RailCar::Groups::create();

    this->railCarShed.push_back(winston::RailCar::make("Bauzug lang", winston::RailCar::Groups::ConstructionTrain, 300.f));
    //this->railCarShed.push_back(winston::RailCar::make("Bauzug doppel", winston::RailCar::Groups::ConstructionTrain, 312));
    //this->railCarShed.push_back(winston::RailCar::make("Bauzug Kran", winston::RailCar::Groups::ConstructionTrain, 300));

    this->railCarShed.push_back(winston::RailCar::make("Personenwagen 1", winston::RailCar::Groups::Person | DR, 250.f));
    //this->railCarShed.push_back(winston::RailCar::make("Personenwagen 2", winston::RailCar::Groups::Person | DR, 250));
    this->railCarShed.push_back(winston::RailCar::make("Gepaeckwagen", winston::RailCar::Groups::Person | DR | winston::RailCar::Groups::CannotBeSingle, 250.f));

    //this->railCarShed.push_back(winston::RailCar::make("Uaai 819", winston::RailCar::Groups::Heavy, 355));

    this->railCarShed.push_back(winston::RailCar::make("Tankwagen lang", winston::RailCar::Groups::Goods, 100.f));
    //this->railCarShed.push_back(winston::RailCar::make("Tankwagen Shell", winston::RailCar::Groups::Goods, 355));

    this->railCarShed.push_back(winston::RailCar::make("Kiara", winston::RailCar::Groups::Goods, 114.f));
    this->railCarShed.push_back(winston::RailCar::make("Alter Gueterwagen", winston::RailCar::Groups::Goods, 114.f));

    this->railCarShed.push_back(winston::RailCar::make("Schiebehaubenwagen", winston::RailCar::Groups::Goods, 146.f));
    this->railCarShed.push_back(winston::RailCar::make("Offener Wagen", winston::RailCar::Groups::Goods, 160.f));

    //this->railCarShed.push_back(winston::RailCar::make("Schwarzer Wagen", winston::RailCar::Groups::Goods, 102));
    //this->railCarShed.push_back(winston::RailCar::make("Brauner Wagen", winston::RailCar::Groups::Goods, 98));
}

void Kornweinheim::inventStorylines()
{
#ifdef WINSTON_STORYLINES

    auto randomScenario = winston::Storyline::make();
    randomScenario->invent([&]() -> winston::Storyline::Task::List {

        winston::Storyline::Task::List tasks;

        // Wagen ... mit ... auf Gleis ... [zusammenstellen|fahren|abstellen]
        tasks.push_back(winston::TaskRandomCars::make(this->railCarShed));
        tasks.push_back(winston::TaskRandomAction::make());
        tasks.push_back(winston::TaskRandomLoco::make(this->locomotiveShed));
        tasks.push_back(winston::TaskRandomSection::make(this->railway->sectionList()));
        tasks.push_back(winston::TaskCallback::make([&](const winston::Storyline::Shared storyline, const winston::Storyline::Task::List& context) -> const winston::State {
#ifdef WINSTON_WITH_WEBSOCKET
            this->webUI.sendStorylineText(storyline);
#endif
            return winston::State::Finished;
            }));
        tasks.push_back(winston::TaskReply::make([](const winston::Storyline::Reply::Answer answer) -> const winston::State {

            switch (answer)
            {
            default: 
            case winston::Storyline::Reply::Answer::None:
                return winston::State::Running;
                break;
            case winston::Storyline::Reply::Answer::Refresh:
                return winston::State::Finished;
            case winston::Storyline::Reply::Answer::Cancel:
                return winston::State::Aborted;
            }
            
            }));

        return tasks;

        }, [](const winston::Storyline::Task::List& context) -> const std::string {

            std::string text;

            const auto railCars = winston::Storyline::get<winston::TaskRandomCars>(context);
            const auto loco = winston::Storyline::get<winston::TaskRandomLoco>(context);
            const auto section = winston::Storyline::get<winston::TaskRandomSection>(context);
            const auto action = winston::Storyline::get<winston::TaskRandomAction>(context);

            if (railCars && loco && section && action)
            {
                // Wagen ... mit ... auf Gleis ... [zusammenstellen|fahren|abstellen]
                text += "Wagen " + railCars->text() + "\n";
                text += "mit " + loco->text() + "\n";
                text += "auf Gleis " + section->text() + "\n";
                text += action->text() + ".";

                return text;
            }
            else
                return "none yet";

        });
    this->storylines.push_back(randomScenario);
    this->activeStory = randomScenario;
#ifdef WINSTON_WITH_WEBSOCKET
    this->webUI.setStoryLine(this->activeStory);
#endif
    this->signalTower->order(winston::Command::make([&](const winston::TimePoint& created) -> const winston::State
        {
            this->activeStory->execute();
            return winston::State::Running;
        }, __PRETTY_FUNCTION__));

    /*
    this->userConfirmation = winston::ConfirmationProvider::make();
    this->display = winston::DisplayLog::make();

    auto collectAndDrive = winston::Storyline::make();
    collectAndDrive->invent([&]() {

        auto loco = winston::TaskRandomLoco::make((unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(loco));
        auto assembleTrack = winston::TaskRandomTrack::make(winston::Section::Type::Transit);
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(assembleTrack));

        auto confirmLocoTrack = winston::TaskConfirm::make(userConfirmation, display, loco, "nach", assembleTrack, [collectAndDrive, loco, assembleTrack]() -> winston::State {
            collectAndDrive->immediate(std::static_pointer_cast<winston::Storyline::Task>(loco));
            collectAndDrive->immediate(std::static_pointer_cast<winston::Storyline::Task>(assembleTrack));
            return winston::State::Running;
        });
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(confirmLocoTrack));

        });
    this->storylines.push_back(collectAndDrive);

    this->signalTower->order(winston::Command::make([](const winston::TimePoint& created) -> const winston::State
        {
            return winston::State::Running;
        }, __PRETTY_FUNCTION__));
        */
#endif

}