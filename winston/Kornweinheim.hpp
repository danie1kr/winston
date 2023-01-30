#ifdef WINSTON_PLATFORM_TEENSY
#define Binary_h
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
*/

#ifdef WINSTON_LOCO_TRACKING
void Kornweinheim::webSocket_sendLocosPosition()
{
#ifdef WINSTON_WITH_WEBSOCKET
    auto now = winston::hal::now();
    if (inMilliseconds(now - this->lastWebsocketLocoTrackingUpdate) > WINSTON_LOCO_UPDATE_POSITION_WEBSOCKET_RATE)
    {
        DynamicJsonDocument obj(32 + this->locomotiveShed.size() * 256);
        obj["op"] = "locoPositions";
        auto data = obj.createNestedArray("data");
        for (const auto &loco: this->locomotiveShed)
        {
            auto l = data.createNestedObject();
            l["address"] = loco.address();

            const auto& pos = loco.position();
            l["track"] = pos.trackName();
            l["connection"] = winston::Track::ConnectionToString(pos.connection());
            l["distance"] = pos.distance();
        }
        std::string json("");
        serializeJson(obj, json);
        webServer.broadcast(json);
        this->lastWebsocketLocoTrackingUpdate = now;
    }
#endif
}
#endif

winston::DigitalCentralStation::Callbacks Kornweinheim::z21Callbacks()
{
    winston::DigitalCentralStation::Callbacks callbacks;

    //
    callbacks.systemInfoCallback = [=](const size_t id, const std::string name, const std::string content) {
        winston::logger.log(winston::build("Z21: ", name, ": ", content));
    };

    //
    callbacks.trackPowerStatusCallback = [=](const bool powerOn) {
        winston::logger.log(std::string("Z21: Power is ") + std::string(powerOn ? "on" : "off"));
    };

    // 
    callbacks.programmingTrackStatusCallback = [=](const bool programmingOn) {
        winston::logger.log(std::string("Z21: Programming is ") + std::string(programmingOn ? "on" : "off"));
    };

    // 
    callbacks.shortCircuitDetectedCallback = [=]() {
        winston::logger.log("Z21: Short circuit detected!");
    };

    // what to do when the digital central station updated a turnout
    callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
    {
        turnout->finalizeChangeTo(direction);
        return winston::State::Finished;
    };

    // what to do when the digital central station updated a turnout
    callbacks.doubleSlipUpdateCallback = [=](winston::DoubleSlipTurnout::Shared turnout, const winston::DoubleSlipTurnout::Direction direction) -> const winston::State
    {
        turnout->finalizeChangeTo(direction);
        return winston::State::Finished;
    };

    callbacks.locomotiveUpdateCallback = [=](winston::Locomotive::Shared loco, bool busy, bool  forward, unsigned char  speed, uint32_t functions)
    {
        loco->update(busy, forward, speed, functions);


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
            auto route = this->railway->route(address - 400);
            if (route)
                this->orderRouteSet(route, true);
        }
        else
        {
            return true;
        }
        
        return false;
    };

    return callbacks;
}

winston::Locomotive::Callbacks Kornweinheim::locoCallbacks()
{
    winston::Locomotive::Callbacks callbacks;

    callbacks.drive = [=](const winston::Address address, const unsigned char speed, const bool forward) {
        this->signalBox->order(winston::Command::make([this, address, speed, forward](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
    };

    callbacks.functions = [=](const winston::Address address, const uint32_t functions) {
        this->signalBox->order(winston::Command::make([this, address, functions](const winston::TimePoint &created) -> const winston::State
            {
                this->digitalCentralStation->triggerLocoFunction(address, functions);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));
    };

    return callbacks;
}

winston::Railway::Callbacks Kornweinheim::railwayCallbacks()
{
    winston::Railway::Callbacks callbacks;

    callbacks.turnoutUpdateCallback = [=](winston::Turnout::Shared turnout, const winston::Turnout::Direction direction) -> const winston::State
    {
        // tell the signal box to update the signals
        this->signalBox->setSignalsFor(turnout);

#ifdef WINSTON_WITH_WEBSOCKET
        // tell the ui what happens
        this->webUI.turnoutSendState(turnout->name(), direction);
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

        winston::logger.info("Turnout ", turnout->name(), " set to direction ", winston::Turnout::DirectionToString(direction));

        return winston::State::Finished;
    };

    callbacks.doubleSlipUpdateCallback = [=](winston::DoubleSlipTurnout::Shared turnout, const winston::DoubleSlipTurnout::Direction direction) -> const winston::State
    {
        // tell the signal box to update the signals
        this->signalBox->setSignalsFor(turnout);

#ifdef WINSTON_WITH_WEBSOCKET
        // tell the ui what happens
        this->webUI.doubleSlipSendState(turnout->name(), direction);
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

        winston::logger.info("Turnout ", turnout->name(), " set to direction ", winston::DoubleSlipTurnout::DirectionToString(direction));

        return winston::State::Finished;
    };

    callbacks.dccDetectorCallback = [=](winston::Detector::Shared, const winston::Address address) -> winston::Result
    {
        // not used here
        return winston::Result::OK;
    };

    return callbacks;
}

#ifdef WINSTON_WITH_WEBSOCKET

winston::Result Kornweinheim::on_http(WebServer::HTTPConnection& connection, const winston::HTTPMethod method, const std::string& resource) {
    const std::string path_railway("/railway");
    const std::string path_log("/log");
    const std::string path_signals("/signals");
    const std::string path_signalstest("/signals-test");
    const std::string path_confirmation_yes("/confirm_yes");
    const std::string path_confirmation_maybe("/confirm_maybe");
    const std::string path_confirmation_no("/confirm_no");

    if (resource.compare(path_railway) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "application/json; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html>winston</html>\r\n"_s);
    }
    else if (resource.compare(path_log) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html><head>winston</head><body><table><tr><th>timestamp</th><th>level</th><th>log</th></tr>"_s);
        for (const auto& entry : winston::logger.entries())
        {
            connection.body("<tr><td>");
            connection.body(winston::build(entry.timestamp)); connection.body("</td><td>");
            connection.body(entry.levelName()); connection.body("</td><td>");
            connection.body(entry.text); connection.body("</td></tr>\r\n");
        }

        connection.body("</table></body></html>\r\n"_s);
    }
    else if (resource.compare(path_signals) == 0)
    {
        connection.status(200);
        connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
        connection.header("Connection"_s, "close"_s);
        connection.body("<html><head>winston signal list</head><body><table border=1><tr><th>track</th><th>connection</th><th>light</th><th>port</th><th>device @ port</th></tr>"_s);
        for (unsigned int i = 0; i < railway->tracksCount(); ++i)
        {
            auto track = railway->track(i);
            switch (track->type())
            {
            case winston::Track::Type::Bumper:
                this->writeSignalHTMLList(connection, track, winston::Track::Connection::DeadEnd);
                this->writeSignalHTMLList(connection, track, winston::Track::Connection::A);
                break;
            case winston::Track::Type::Rail:
                this->writeSignalHTMLList(connection, track, winston::Track::Connection::A);
                this->writeSignalHTMLList(connection, track, winston::Track::Connection::B);
                break;
            default:
                break;
            }
        }
        connection.body("</table></body></html>\r\n"_s);
    }
    else if (resource.compare(path_signalstest) == 0)
    {
        const unsigned int interval = 5;
        signalBox->order(winston::Command::make([this, interval](const winston::TimePoint& created) -> const winston::State
            {
                if (winston::hal::now() - created > std::chrono::seconds(5* interval))
                {
                    winston::logger.info("Signal-Test: Resume");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)0);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Finished;
                }
                else if (winston::hal::now() - created > std::chrono::seconds(4* interval))
                {
                    winston::logger.info("Signal-Test: ExpectGo");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)winston::Signal::Aspect::ExpectGo);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Running;
                }
                else if (winston::hal::now() - created > std::chrono::seconds(3* interval))
                {
                    winston::logger.info("Signal-Test: ExpectHalt");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)winston::Signal::Aspect::ExpectHalt);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Running;
                }
                else if (winston::hal::now() - created > std::chrono::seconds(2*interval))
                {
                    winston::logger.info("Signal-Test: Halt");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)winston::Signal::Aspect::Halt);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Running;
                }
                else if (winston::hal::now() - created > std::chrono::seconds(interval))
                {
                    winston::logger.info("Signal-Test: Go");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)winston::Signal::Aspect::Go);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Running;
                }
                else
                {
                    winston::logger.info("Signal-Test: Off");
                    for (auto& s : this->signals)
                    {
                        s->overwrite((const unsigned int)winston::Signal::Aspect::Off);
                        this->signalDevice->update(s);
                    }
                    return winston::State::Running;
                }
            }));
            connection.status(200);
            connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
            connection.header("Connection"_s, "close"_s);
            connection.body(winston::build("<html><head>winston signal test</head><body>signal test for 5x ", interval, "s </body></html>"_s));
}
    else if (resource.compare(path_confirmation_yes) == 0)
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
    else
    {
#ifdef WINSTON_PACKED_WEBFILES
        const auto file = resource.substr(0, resource.find("?"));
        auto resourceFile = resources::get(file);
        if (resourceFile != nullptr)
        {
            connection.status(200);
            if(resourceFile->content_type.compare("font/woff2") == 0)
                connection.header("content-type"_s, "application/octet-stream");// +"; charset = UTF - 8"_s);
            else
                connection.header("content-type"_s, resourceFile->content_type);// +"; charset = UTF - 8"_s);
            //connection.header("Connection"_s, "close"_s);
            connection.body(resourceFile->data, resourceFile->size, 64);
        }
        else
#endif
            return winston::Result::NotFound;
    }

    return winston::Result::OK;
}

// add a signal to the /signal output
void Kornweinheim::writeSignalHTMLList(WebServer::HTTPConnection& connection, const winston::Track::Shared track, const winston::Track::Connection trackCon)
{
    if (auto signal = track->signalGuarding(trackCon))
    {
        unsigned int l = 0;
        size_t cnt = signal->lights().size();
        for (const auto& light : signal->lights())
        {
            std::string icon = "", color = "black";
            switch (light.aspect)
            {
            default:
            case winston::Signal::Aspect::Off:
                icon = "off";
                break;
            case winston::Signal::Aspect::Halt:
                icon = "&#11044;";
                if (signal->shows(light.aspect))
                {
                    color = "red";
                };
                break;
            case winston::Signal::Aspect::Go:
                icon = "&#11044;";
                if (signal->shows(light.aspect)) {
                    color = "green";
                }; break;
            case winston::Signal::Aspect::ExpectHalt:
                icon = "&#9675;";
                if (signal->shows(light.aspect)) {
                    color = "red";
                };
                break;
            case winston::Signal::Aspect::ExpectGo:
                icon = "&#9675;";
                if (signal->shows(light.aspect)) {
                    color = "green";
                }; break;
            }

            std::string signal = "<span style=\"color:" + color + ";\">" + icon + "</span>";

            if (l == 0)
                connection.body(winston::build("<tr><td rowspan=", cnt, ">", track->name(), "</td><td rowspan=", cnt, ">", winston::Track::ConnectionToString(trackCon), "</td><td>"));
            else
                connection.body(winston::build("<tr><td>"));
            ++l;
            connection.body(winston::build(l, signal, "</td><td>", light.port, "</td><td>", light.port == 999 ? "n/a" : winston::build(light.port / 24, " @ ", light.port % 24), "</td></tr>"));
        }
    }
}
#endif

const winston::Result Kornweinheim::orderRouteSet(winston::Route::Shared route, const bool set)
{
    auto state = route->set(set);
    if (state == winston::Route::State::Setting)
    {
        this->routesInProgress.push_back(route);

        for (auto conflicting : route->getConflictingRoutes())
        {
            conflicting->disable(true);
#ifdef WINSTON_WITH_WEBSOCKET
            this->webUI.routeState(*conflicting);
#endif
        }

        route->eachTurnout<true, true>([this](const winston::Route::Turnout& turnout)
            {
                this->orderTurnoutToggle(turnout.turnout(), turnout.direction);
            },
            [this](const winston::Route::DoubleSlipTurnout& turnout)
            {
                this->orderDoubleSlipTurnoutToggle(turnout.doubleSlipTurnout(), turnout.direction);
            }
            );
    }
    else if (state == winston::Route::State::Unset)
    {
        this->railway->evaluateConflictingRoutes([this](winston::Route::Shared& route)
            {
#ifdef WINSTON_WITH_WEBSOCKET
                signalBox->order(winston::Command::make([this, route](const winston::TimePoint& created) -> const winston::State
                    {
                        this->webUI.routeState(*route);
                        return winston::State::Finished;
                    }, __PRETTY_FUNCTION__));
#endif
            });
    }
#ifdef WINSTON_WITH_WEBSOCKET
    this->webUI.routeState(*route);
#endif

    return winston::Result::OK;
}

void Kornweinheim::orderTurnoutToggle(winston::Turnout::Shared turnout, winston::Turnout::Direction direction)
{
    this->signalBox->order(winston::Command::make([this, turnout, direction](const winston::TimePoint& created) -> const winston::State
        {
            WINSTON_TURNOUT_TOGGLE_GUARD;

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
            this->signalBox->order(winston::Command::make([this, turnout, direction](const winston::TimePoint& created) -> const winston::State
                {
                    if (inMilliseconds((winston::hal::now() - created)) > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
                    {
                        this->stationDebugInjector->injectTurnoutUpdate(turnout, direction);
                        return winston::State::Finished;
                    }
    return winston::State::Delay;
                }, __PRETTY_FUNCTION__));
#endif
    // tell the central station to trigger the turnout switch
    // update internal representation. will inform the UI in its callback, too
    return this->turnoutChangeTo(turnout, direction);
        }, __PRETTY_FUNCTION__));
}

void Kornweinheim::orderDoubleSlipTurnoutToggle(winston::DoubleSlipTurnout::Shared turnout, winston::DoubleSlipTurnout::Direction direction)
{
    this->signalBox->order(winston::Command::make([this, turnout, direction](const winston::TimePoint& created) -> const winston::State
        {
            WINSTON_TURNOUT_TOGGLE_GUARD;

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
            this->signalBox->order(winston::Command::make([this, turnout, direction](const winston::TimePoint& created) -> const winston::State
                {
                    if (inMilliseconds((winston::hal::now() - created)) > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
                    {
                        this->stationDebugInjector->injectDoubleSlipTurnoutUpdate(turnout, direction);
                        return winston::State::Finished;
                    }
    return winston::State::Delay;
                }, __PRETTY_FUNCTION__));
#endif
    // tell the central station to trigger the turnout switch
    // update internal representation. will inform the UI in its callback, too
    return this->doubleSlipChangeTo(turnout, direction);
    }, __PRETTY_FUNCTION__));
}

// setup our model railway system
void Kornweinheim::systemSetup() {
    // the user defined railway and its address translator
    this->railway = RAILWAY_CLASS::make(railwayCallbacks());
    this->populateLocomotiveShed();

    // z21
    z21Socket = UDPSocket::make(z21IP, z21Port);

    this->addressTranslator = RAILWAY_CLASS::AddressTranslator::make(railway);

    // the internal signal box
    this->signalBox = winston::SignalBox::make();

    // the system specific digital central station
    auto at = std::static_pointer_cast<winston::DigitalCentralStation::TurnoutAddressTranslator>(addressTranslator);
    auto udp = std::static_pointer_cast<winston::hal::Socket>(this->z21Socket);
    this->digitalCentralStation = Z21::make(udp, at, *this, this->signalBox, z21Callbacks());

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    // a debug injector
    auto dcs = std::static_pointer_cast<winston::DigitalCentralStation>(this->digitalCentralStation);
    this->stationDebugInjector = winston::DigitalCentralStation::DebugInjector::make(dcs);
#endif

    // signals
#ifdef WINSTON_PLATFORM_TEENSY
    this->signalInterfaceDevice = SignalInterfaceDevice::make(40, TLC5947_SignalDevice::SPI_Clock);
    auto TLC5947Off = Arduino_GPIOOutputPin::make(41, Arduino_GPIOOutputPin::State::High);
#else
    this->signalInterfaceDevice = SignalInterfaceDevice::make(3, TLC5947_SignalDevice::SPI_Clock);
    auto TLC5947Off = this->signalInterfaceDevice->getOutputPinDevice(4);
#endif
    this->signalInterfaceDevice->init();
    this->signalInterfaceDevice->skipSend(true);
    const unsigned int chainedTLC5947s = 4;
    this->signalDevice = TLC5947_SignalDevice::make(chainedTLC5947s * 24, this->signalInterfaceDevice, TLC5947Off);

    // storage
    this->storageLayout = Storage::make(std::string(this->name()).append(".").append("winston.storage"), 128 * 1024);
    if (this->storageLayout->init() != winston::Result::OK)
        winston::logger.err("Kornweinheim.init: Storage Layout Init failed");

    // detectors
#ifdef WINSTON_PLATFORM_TEENSY

#elif defined(WINSTON_PLATFORM_WIN_x64)
    this->serial = SerialDeviceWin::make();
    this->serial->init(5);
#endif
    this->routesInProgress.clear();
    this->inventStorylines();

    this->webUI.init(this->railway, this->locomotiveShed, this->storageLayout, this->addressTranslator, this->digitalCentralStation, 8080,
        [=](WebServer::HTTPConnection& client, const winston::HTTPMethod method, const std::string& resource) -> winston::Result {
            return this->on_http(client, method, resource);
        },
        [=](const std::string id) -> winston::Result {
            auto track = railway->track(id);
            if (track->type() == winston::Track::Type::Turnout)
            {
                auto turnout = std::static_pointer_cast<winston::Turnout>(track);
                auto requestDir = winston::Turnout::otherDirection(turnout->direction());
                this->orderTurnoutToggle(turnout, requestDir);
            }
            else if(track->type() == winston::Track::Type::DoubleSlipTurnout)
            {
                auto turnout = std::static_pointer_cast<winston::DoubleSlipTurnout>(track);
                auto requestDir = winston::DoubleSlipTurnout::nextDirection(turnout->direction());
                this->orderDoubleSlipTurnoutToggle(turnout, requestDir);
            }
            return winston::Result::OK;
        },
        [=](const int id, const bool set) -> winston::Result {
            auto route = railway->route(id);
            if (!route)
                return winston::Result::NotFound;

            return this->orderRouteSet(route, set);
        }
    );

    winston::logger.setCallback([this](const winston::Logger::Entry& entry) {
        this->webUI.log(entry);
    });
};

void Kornweinheim::setupSignals()
{
    auto signalUpdateCallback = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
#ifdef WINSTON_WITH_WEBSOCKET
        // send to web socket server
        this->webUI.signalSendState(track->name(), connection, aspects);
#endif

        // update physical light
        this->signalDevice->update(track->signalGuarding(connection));
        winston::logger.info("Signal at ", track->name(), "|", winston::Track::ConnectionToString(connection), " set to ", aspects);

        return winston::State::Finished;
    };

    auto signalUpdateAlwaysHalt = [=](winston::Track::Shared track, winston::Track::Connection connection, const winston::Signal::Aspects aspects) -> const winston::State
    {
        return winston::State::Finished;
    };

    // H
    unsigned int port = 0;
    // PBF
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF1), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF2), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF3), winston::Track::Connection::B, 5U, port, signalUpdateCallback));

    // dummy
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF1), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF2), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::PBF3), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::PBF1a), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    ++port; // to align with 24port device

    // rechte Strecke
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B1), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B4), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B1), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B4), winston::Track::Connection::A, 5U, port, signalUpdateCallback));

    // obere Strecke
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B2), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B5), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B2), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B5), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    // linke Strecke
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B3), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::B6), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    ++port; // to align with 24port device
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B3), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::B6), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    
    // Abstellgleise
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::N2), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    
    // GBF
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::GBF1), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF3a), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::GBF3b), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF3b), winston::Track::Connection::B, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF2a), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    ++port; // to align with 24port device
    this->signals.push_back(this->signalFactory<winston::SignalKS>(this->railway->track(Y2021RailwayTracks::GBF2b), winston::Track::Connection::A, 5U, port, signalUpdateCallback));
    this->signals.push_back(this->signalFactory<winston::SignalH>(this->railway->track(Y2021RailwayTracks::GBF2b), winston::Track::Connection::B, 5U, port, signalUpdateCallback));

    // don't care for ports here
    unsigned int dncPort = 999;
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::N2), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::PBF1a), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::GBF1), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::GBF2a), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
    dncPort = 999; this->signalFactory<winston::SignalAlwaysHalt>(this->railway->track(Y2021RailwayTracks::GBF3a), winston::Track::Connection::DeadEnd, 5U, dncPort, signalUpdateAlwaysHalt);
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
    */
    auto now = winston::hal::now(); // - offset (=2 ms Card->IC, +1ms IC->Teensy, +5ms Teensy->Teensy == 8ms <=> 2.04mm @ scaled 0.255 m/s @ 22.2m/s = 80 km/h
    // actual work as we do not want to block the receiver
    this->signalBox->order(winston::Command::make([detector, loco, now](const winston::TimePoint& created) -> const winston::State
        {
            // loco is now at
            auto current = detector->position();
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

            this->signalBox->order(winston::Command::make([=](const winston::TimePoint &created) -> const winston::State
            {
                railway -> updateBlockOccupancy(previous, pos, timeOnTour, loco.trainLength() = 100);
                return winston::State::Finished;
            }, __PRETTY_FUNCTION__));

            this->signalBox->order(winston::Command::make([signalBox](const winston::TimePoint &created) -> const winston::State
            {
                signalbox -> updateSignalsAccordingToBlocks();
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

void Kornweinheim::setupDetectors()
{
}

void Kornweinheim::systemSetupComplete()
{
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
    this->stationDebugInjector->injectConnected();

    this->railway->turnouts([=](const Tracks track, winston::Turnout::Shared turnout) {
        this->stationDebugInjector->injectTurnoutUpdate(turnout, std::rand() % 2 ? winston::Turnout::Direction::A_B : winston::Turnout::Direction::A_C);
        });
    this->railway->doubleSlipTurnouts([=](const Tracks track, winston::DoubleSlipTurnout::Shared turnout) {
        auto v = std::rand() % 4;
        auto dir = winston::DoubleSlipTurnout::Direction::A_B;
        if (v == 1) dir = winston::DoubleSlipTurnout::Direction::A_C;
        else if (v == 2) dir = winston::DoubleSlipTurnout::Direction::B_D;
        else if (v == 3) dir = winston::DoubleSlipTurnout::Direction::C_D;
        this->stationDebugInjector->injectDoubleSlipTurnoutUpdate(turnout, dir);
        });
 #endif
    this->signalInterfaceDevice->skipSend(false);
    this->signalDevice->flush();
    this->signalBox->order(this->signalDevice->flushCommand());
}

// accept new requests and loop over what the signal box has to do
bool Kornweinheim::systemLoop()
{    
    {
#ifdef WINSTON_WITH_WEBSOCKET
#ifdef WINSTON_STATISTICS
        winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "webServer");
#endif
        this->webUI.step();
#endif
    }
    {
#ifdef WINSTON_LOCO_TRACKING
#ifdef WINSTON_STATISTICS
        StopwatchJournal::Event tracer(this->stopWatchJournal, "loco tracking");
#endif
        this->webSocket_sendLocosPosition();
#endif
    }
#ifdef WINSTON_STATISTICS
    winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "signalBox");
#endif
    return this->signalBox->work();
}

void Kornweinheim::populateLocomotiveShed()
{
    auto callbacks = locoCallbacks();
    winston::Position pos(this->railway->track(Y2021RailwayTracks::N1), winston::Track::Connection::A, 100);

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


    this->addLocomotive(callbacks, 3, functionsBR114, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 114", (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods | (unsigned char)winston::Locomotive::Type::Shunting);
    this->addLocomotive(callbacks, 4, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 106", (unsigned char)winston::Locomotive::Type::Shunting | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 5, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 64", (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 6, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "E 11", (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 8, standardFunctions, pos, winston::Locomotive::defaultThrottleSpeedMap, "BR 218", (unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
    this->addLocomotive(callbacks, 7, functionsGravita, pos, winston::Locomotive::defaultThrottleSpeedMap, "Gravita", (unsigned char)winston::Locomotive::Type::Shunting | (unsigned char)winston::Locomotive::Type::Goods);
}

void Kornweinheim::inventStorylines()
{
#ifdef WINSTON_STORYLINES
    this->userConfirmation = winston::ConfirmationProvider::make();
    this->display = winston::DisplayLog::make();

    auto collectAndDrive = winston::Storyline::make();
    collectAndDrive->invent([&]() {

        auto loco = winston::TaskRandomLoco::make((unsigned char)winston::Locomotive::Type::Passenger | (unsigned char)winston::Locomotive::Type::Goods);
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(loco));
        auto assembleTrack = winston::TaskRandomTrack::make(winston::Block::Type::Transit);
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(assembleTrack));

        auto confirmLocoTrack = winston::TaskConfirm::make(userConfirmation, display, loco, "nach", assembleTrack, [collectAndDrive, loco, assembleTrack]() -> winston::State {
            collectAndDrive->immediate(std::static_pointer_cast<winston::Storyline::Task>(loco));
            collectAndDrive->immediate(std::static_pointer_cast<winston::Storyline::Task>(assembleTrack));
            return winston::State::Running;
        });
        collectAndDrive->queue(std::static_pointer_cast<winston::Storyline::Task>(confirmLocoTrack));

        });
    this->storylines.push_back(collectAndDrive);

    this->signalBox->order(winston::Command::make([](const winston::TimePoint& created) -> const winston::State
        {
            return winston::State::Running;
        }, __PRETTY_FUNCTION__));
#endif
}