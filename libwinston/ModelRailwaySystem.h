#pragma once

#ifdef WINSTON_PLATFORM_TEENSY
//#include "pgmspace.h"
#else
//#define FLASHMEM
#endif

#include "WinstonConfig.h"
#include "HAL.h"
#include "Railway.h"
#include "SignalTower.h"
#include "Log.h"
#include "DigitalCentralStation.h"
#include "WebServer.h"

#ifdef WINSTON_TURNOUT_TOGGLE_INTERVAL
#define WINSTON_TURNOUT_TOGGLE_GUARD	\
if (winston::hal::now() - this->lastTurnoutToggleRequest < WINSTON_TURNOUT_TOGGLE_INTERVAL) \
	return winston::State::Delay;	\
lastTurnoutToggleRequest = winston::hal::now();
#else
#define WINSTON_TURNOUT_TOGGLE_GUARD	;
#endif

namespace winston
{
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation, class _WebServer>
	class ModelRailwaySystem : public DigitalCentralStation::LocoAddressTranslator
	{
	public:
#ifdef WINSTON_WITH_WEBSOCKET
		using Web = winston::WebUI<_WebServer, _Railway>;
#endif

		ModelRailwaySystem()
			: _status{ Status::Initializing }
			, lastStatusReport { hal::now() }
#ifdef WINSTON_STATISTICS
			, stopWatchJournal("MRS")
#endif
		{ };
		virtual ~ModelRailwaySystem() { } ;

	private:
		TimePoint lastDCSConnectedCheck;
	public:

		void setup() {
			TEENSY_CRASHLOG_BREADCRUMB(2, 0x1);
			LOG_INFO("System: Setup");
			this->systemSetup();

			TEENSY_CRASHLOG_BREADCRUMB(2, 0x2);
			LOG_INFO("System: Railway Init");
			this->railway->init();/*

			for(const auto &loco : this->locomotiveShed ) {

				this->signalTower->order(winston::Command::make([this, loco](const TimePoint &created) -> const winston::State
					{
						this->digitalCentralStation->requestLocoInfo(loco);
						winston::hal::delay(150);
						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
			}*/

			TEENSY_CRASHLOG_BREADCRUMB(2, 0x3);
			LOG_INFO("System: Signal Setup");
			this->setupSignals();
			this->setupNextSignals();

			TEENSY_CRASHLOG_BREADCRUMB(2, 0x4);
			LOG_INFO("System: Detector Setup");
			if(this->setupDetectors() != Result::OK)
				LOG_ERROR("Could not set up Detectors.");

			TEENSY_CRASHLOG_BREADCRUMB(2, 0x5);
			LOG_INFO("System: Railway Final Validation");
			this->railway->validateFinal();
#ifdef WINSTON_REALWORLD
			this->signalTower->order(winston::Command::make([this](const TimePoint& created) -> const winston::State
				{
					if (winston::hal::now() - this->lastDCSConnectedCheck > 2000ms)
					{
						if (!this->digitalCentralStation->connected())
						{
							LOG_INFO("Connecting to DigitalCentralStation.");
							this->digitalCentralStation->connect();
						}
						//else
						//	LOG_ERROR("Connection to DigitalCentralStation lost.");

						this->lastDCSConnectedCheck = winston::hal::now();
					}
					return winston::State::Delay;
		}, __PRETTY_FUNCTION__));
			;
#else
			LOG_WARN("not connecting to digitalCentralStation as WINSTON_REALWORLD is not defined");
#endif
			
			this->signalTower->order(winston::Command::make([](const TimePoint &created) -> const winston::State
				{
					LOG_INFO("Init tasks complete");
					return winston::State::Finished;
				}, __PRETTY_FUNCTION__));

			TEENSY_CRASHLOG_BREADCRUMB(2, 0x6);
			this->systemSetupComplete();
			LOG_INFO("System: Setup complete");
			TEENSY_CRASHLOG_BREADCRUMB(2, 0x7);
			this->_status = Status::Ready;
		};
#ifdef WINSTON_HAL_USE_WEBSERVER
		void setupWebServer(winston::hal::StorageInterface::Shared storageLayout, winston::hal::StorageInterface::Shared storageMicroLayout, typename _Railway::AddressTranslator::Shared addressTranslator, const unsigned int port
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
			, typename winston::WebUI<_WebServer, _Railway>::DetectorInjectionCallback detectorInjectionCallback
#endif
		)
		{
			this->webUI.init(this->railway, this->locomotiveShed, storageLayout, storageMicroLayout, addressTranslator, this->digitalCentralStation, port,
				[=](typename _WebServer::HTTPConnection& client, const winston::HTTPMethod method, const std::string& resource) -> winston::Result {
					auto result = this->on_http_internal(client, method, resource);
			if (result == Result::NotFound)
				return this->on_http(client, method, resource);
			return result;
				},
				[=](const std::string id) -> winston::Result {
					auto track = railway->track(id);
				if (track->type() == winston::Track::Type::Turnout)
				{
					auto turnout = std::static_pointer_cast<winston::Turnout>(track);
					auto requestDir = winston::Turnout::otherDirection(turnout->direction());
					this->orderTurnoutToggle(*turnout, requestDir);
				}
				else if (track->type() == winston::Track::Type::DoubleSlipTurnout)
				{
					auto turnout = std::static_pointer_cast<winston::DoubleSlipTurnout>(track);
					auto requestDir = winston::DoubleSlipTurnout::nextDirection(turnout->direction());
					this->orderDoubleSlipTurnoutToggle(*turnout, requestDir);
				}
				return winston::Result::OK;
				},
					[=](const int id, const bool set) -> winston::Result {
					auto route = railway->route(id);
				if (!route)
					return winston::Result::NotFound;

				return this->orderRouteSet(route, set);
				}
#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
				, detectorInjectionCallback
#endif
				);
		}
#endif
		static const std::string name()
		{
			return _Railway::name();
		}

		Locomotive::Shared locoFromAddress(const Address address)
		{
			auto locos = this->locomotiveShed.shed();
			auto it = std::find_if(locos.begin(), locos.end(), [address](const auto& loco) { return loco->address() == address; });
			if (it == locos.end())
				return nullptr;
			else
				return *it;
		}

		const Address addressOfLoco(const Locomotive& loco) const
		{
			return loco.address();
		}

		inline const State turnoutChangeTo(winston::Turnout& turnout, winston::Turnout::Direction direction)
		{
			this->digitalCentralStation->triggerTurnoutChangeTo(turnout, direction);
			return turnout.startToggle();
		}

		inline const State doubleSlipChangeTo(winston::DoubleSlipTurnout& turnout, winston::DoubleSlipTurnout::Direction direction)
		{
			this->digitalCentralStation->triggerDoubleSlipTurnoutChangeTo(turnout, direction);
			return turnout.startToggle();
		}

		inline const State locoFunction(const winston::Address address, const uint32_t functions)
		{
			this->digitalCentralStation->triggerLocoFunction(address, functions);
			return winston::State::Finished;
		}

		inline const State locoDrive(const Address address, const unsigned char speed, const bool forward)
		{
			this->digitalCentralStation->triggerLocoDrive(address, speed, forward);
			return winston::State::Finished;
		}

		void digitalCentralStationConnected()
		{
			this->railway->turnouts([=](const Tracks track, winston::Turnout& turnout) {

				checkedTurnoutsDuringInit.emplace(turnout.shared_from_this(), false);
				this->signalTower->order(winston::Command::make([this, &turnout](const TimePoint& created) -> const winston::State
					{
						this->digitalCentralStation->requestTurnoutInfo(turnout);
						winston::hal::delay(50);
						this->signalTower->setSignalsFor(turnout);
						winston::hal::delay(100);

						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
				});

			this->railway->doubleSlipTurnouts([=](const Tracks track, winston::DoubleSlipTurnout& turnout) {

				checkedTurnoutsDuringInit.emplace(turnout.shared_from_this(), false);
				this->signalTower->order(winston::Command::make([this, &turnout](const TimePoint& created) -> const winston::State
					{
						this->digitalCentralStation->requestDoubleSlipTurnoutInfo(turnout);
						winston::hal::delay(50);
						this->signalTower->setSignalsFor(turnout);
						winston::hal::delay(100);

						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
				});

			for (const auto& loco : this->locomotiveShed.shed()) {

				this->signalTower->order(winston::Command::make([this, &loco](const TimePoint& created) -> const winston::State
					{
						this->digitalCentralStation->requestLocoInfo(*loco);
						winston::hal::delay(150);
						return winston::State::Finished;
					}, __PRETTY_FUNCTION__));
			}
		}

		std::vector<winston::Route::Shared> routesInProgress;
		const winston::Result orderRouteSet(winston::Route::Shared route, const bool set)
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

				route->eachTurnout<true, true>([this, id = route->id](const winston::Route::Turnout& turnout)
					{
						this->orderTurnoutToggle(*turnout.turnout(), turnout.direction);
						turnout.turnout()->lock(id);
					},
					[this, id = route->id](const winston::Route::DoubleSlipTurnout& turnout)
					{
						this->orderDoubleSlipTurnoutToggle(*turnout.doubleSlipTurnout(), turnout.direction);
						turnout.doubleSlipTurnout()->lock(id);
					}
					);
			}
			else if (state == winston::Route::State::Unset)
			{
				this->railway->evaluateConflictingRoutes([this](winston::Route::Shared& route)
					{
#ifdef WINSTON_WITH_WEBSOCKET
						signalTower->order(winston::Command::make([this, route](const winston::TimePoint& created) -> const winston::State
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
		
		void orderTurnoutToggle(winston::Turnout& turnout, winston::Turnout::Direction direction)
		{
			this->signalTower->order(winston::Command::make([this, &turnout, direction](const winston::TimePoint& created) -> const winston::State
				{
					WINSTON_TURNOUT_TOGGLE_GUARD;

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
			this->signalTower->order(winston::Command::make([this, &turnout, direction](const winston::TimePoint& created) -> const winston::State
				{
					if (winston::hal::now() - created > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
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
		};

		void orderDoubleSlipTurnoutToggle(winston::DoubleSlipTurnout& turnout, winston::DoubleSlipTurnout::Direction direction)
		{
			this->signalTower->order(winston::Command::make([this, &turnout, direction](const winston::TimePoint& created) -> const winston::State
				{
					WINSTON_TURNOUT_TOGGLE_GUARD;

#ifdef WINSTON_RAILWAY_DEBUG_INJECTOR
			this->signalTower->order(winston::Command::make([this, &turnout, direction](const winston::TimePoint& created) -> const winston::State
				{
					if (winston::hal::now() - created > WINSTON_RAILWAY_DEBUG_INJECTOR_DELAY)
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
		};

#ifdef WINSTON_WITH_WEBSOCKET
		Web webUI;

		// Define a callback to handle incoming messages

		virtual const Result on_http(typename _WebServer::HTTPConnection& connection, const HTTPMethod method, const std::string& resource) = 0;
		const Result on_http_internal(typename _WebServer::HTTPConnection& connection, const HTTPMethod method, const std::string& resource) {
			const std::string path_signals("/signals");
			const std::string path_locos("/locos");
			const std::string path_signalstest("/signals-test");
			const std::string path_confirmation_yes("/confirm_yes");
			const std::string path_confirmation_maybe("/confirm_maybe");
			const std::string path_confirmation_no("/confirm_no");
			TEENSY_CRASHLOG_BREADCRUMB(3, 0x1);

			if (resource.compare(path_signals) == 0)
			{
				TEENSY_CRASHLOG_BREADCRUMB(3, 0x100);
				connection.status(200);
				connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
				connection.header("Connection"_s, "close"_s);
				std::string body = "<html><head>winston signal list</head><body><table border=1><tr><th>track</th><th>connection</th><th>light</th><th>port</th><th>device @ port</th></tr>";
				connection.body(body); body = "";
				for (unsigned int i = 0; i < railway->tracksCount(); ++i)
				{
					auto track = railway->track(i);
					switch (track->type())
					{
					case winston::Track::Type::Bumper:
						body = this->writeSignalHTMLList(track, winston::Track::Connection::DeadEnd); connection.body(body);
						body = this->writeSignalHTMLList(track, winston::Track::Connection::A); connection.body(body);
						break;
					case winston::Track::Type::Rail:
						body = this->writeSignalHTMLList(track, winston::Track::Connection::A); connection.body(body);
						body = this->writeSignalHTMLList(track, winston::Track::Connection::B); connection.body(body);
						break;
					default:
						break;
					}
				}
				body = "</table></body></html>\r\n";
				connection.body(body);
				connection.submit();
			}
			else if (resource.compare(path_locos) == 0)
			{
				TEENSY_CRASHLOG_BREADCRUMB(3, 0x101);
				connection.status(200);
				connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
				connection.header("Connection"_s, "close"_s);
				std::string body = "<html><head>winston loco list</head><body><table border=1><tr><th>loco</th><th>speed map</th></tr>";
				connection.body(body);
				auto shed = locomotiveShed.shed();
				for (unsigned int i = 0; i < shed.size(); ++i)
				{
					auto loco = shed.at(i);
					connection.body(build("<tr><td>", loco->name(), "<br> DCC: ", loco->address(), "</td><td>"));

					connection.body("<table><tr><th>Throttle</th><th>Speed</th></tr>");
					loco->eachSpeedMap([&connection](const Throttle throttle, const Speed speed)
						{
							connection.body(build("<tr><td>", throttle, "</td><td>", speed, " mm/s</td></tr>"));
						});
					connection.body("</table></td></tr>");
					
					connection.body("</td></tr>");
				}
				connection.body("</table></body></html>\r\n");
				connection.submit();
			}
			/*
			else if (resource.compare(path_signalstest) == 0)
			{
				const unsigned int interval = 5;
				signalTower->order(winston::Command::make([this, interval](const winston::TimePoint& created) -> const winston::State
					{
						if (winston::hal::now() - created > std::chrono::seconds(5 * interval))
						{
							LOG_INFO("Signal-Test: Resume");
							for (auto& s : this->signals)
							{
								s->overwrite((const unsigned int)0);
								this->signalDevice->update(s);
							}
							return winston::State::Finished;
						}
						else if (winston::hal::now() - created > std::chrono::seconds(4 * interval))
						{
							LOG_INFO("Signal-Test: ExpectGo");
							for (auto& s : this->signals)
							{
								s->overwrite((const unsigned int)winston::Signal::Aspect::ExpectGo);
								this->signalDevice->update(s);
							}
							return winston::State::Running;
						}
						else if (winston::hal::now() - created > std::chrono::seconds(3 * interval))
						{
							LOG_INFO("Signal-Test: ExpectHalt");
							for (auto& s : this->signals)
							{
								s->overwrite((const unsigned int)winston::Signal::Aspect::ExpectHalt);
								this->signalDevice->update(s);
							}
							return winston::State::Running;
						}
						else if (winston::hal::now() - created > std::chrono::seconds(2 * interval))
						{
							LOG_INFO("Signal-Test: Halt");
							for (auto& s : this->signals)
							{
								s->overwrite((const unsigned int)winston::Signal::Aspect::Halt);
								this->signalDevice->update(s);
							}
							return winston::State::Running;
						}
						else if (winston::hal::now() - created > std::chrono::seconds(interval))
						{
							LOG_INFO("Signal-Test: Go");
							for (auto& s : this->signals)
							{
								s->overwrite((const unsigned int)winston::Signal::Aspect::Go);
								this->signalDevice->update(s);
							}
							return winston::State::Running;
						}
						else
						{
							LOG_INFO("Signal-Test: Off");
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
				connection.submit();
			}*/
			else
				return Result::NotFound;
#if 1
#else
			connection.status(200);
			connection.header("content-type"_s, "text/html; charset=UTF-8"_s);
			connection.header("Connection"_s, "close"_s);
			std::string body = "<html><head>winston</head><body>no content</body></html>";
			connection.body(body);
			connection.submit();
#endif
			return Result::OK;
		}

		// add a signal to the /signal output
		const std::string writeSignalHTMLList(const Track::Shared track, const Track::Connection trackCon)
		{
			std::string body;
			if (auto signal = track->signalGuarding(trackCon))
			{
				unsigned int l = 0;
				size_t cnt = signal->lights().size();
				for (const auto& light : signal->lights())
				{
					std::string icon = "", color = "black", aspect = winston::build(light.aspect);
					
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

					//std::string signal = "<span style=\"color:" + color + ";\">" + icon + "</span>";

					if (l == 0)
						body += winston::build("<tr><td rowspan=", cnt, ">", track->name(), "</td><td rowspan=", cnt, ">", winston::Track::ConnectionToString(trackCon), "</td><td>");
					else
						body += winston::build("<tr><td>");
					++l;
					body += winston::build(l, "<span style=\"color:" + color + ";\">" + icon + " " + aspect + "</span>", "</td><td>", light.port, "</td><td>", signal->deviceId, " @ ", light.port < 10 ? "0" : "", light.port, "</td></tr>");
				}
			}
			return body;
		}
#endif

		const std::chrono::seconds statusReportTimeout = std::chrono::seconds(24);
		TimePoint lastStatusReport;
		const Result loop()
		{
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x1);
#ifdef WINSTON_STATISTICS
				StopwatchJournal::Event tracer(this->stopWatchJournal, "digitalCentralStation loop");
#endif
				this->digitalCentralStation->loop();
			}
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x4);
#ifdef WINSTON_STATISTICS
				StopwatchJournal::Event tracer(this->stopWatchJournal, "detector network device");
#endif
				if (this->detectorDevice)
					this->detectorDevice->loop();
			}
			/* {
			* // doen in kornweinheim.loop
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x2);
#ifdef WINSTON_LOCO_TRACKING
#ifdef WINSTON_STATISTICS
				StopwatchJournal::Event tracer(this->stopWatchJournal, "loco tracking");
#endif
				for (auto& loco : this->locomotiveShed.shed())
					loco->update();
#endif
			}
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x3);
#ifdef WINSTON_LOCO_TRACKING
#ifdef WINSTON_STATISTICS
				StopwatchJournal::Event tracer(this->stopWatchJournal, "loco tracking");
#endif
				this->webSocket_sendLocosPosition();
#endif
			}*/
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x5);
#ifdef WINSTON_STATISTICS
				winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "local loop");
#endif
				this->systemLoop();
			}
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x6);
#ifdef WINSTON_STATISTICS
				winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "local status report");
#endif
				const auto now = hal::now();
				if (now > this->lastStatusReport + statusReportTimeout)
				{
					const bool detectorConnected = this->detectorDevice->connected();
					const bool dcsConnected = this->digitalCentralStation->connected();
					if (!detectorConnected)
						LOG_ERROR("Detector Device not connected");
					if (!dcsConnected)
						LOG_ERROR("Digital Central Station not connected");
					this->lastStatusReport = now;
				}
			}
			{
				TEENSY_CRASHLOG_BREADCRUMB(4, 0x7);
#ifdef WINSTON_STATISTICS
				winston::StopwatchJournal::Event tracer(this->stopWatchJournal, "signalTower");
#endif
				return this->signalTower->loop();
			}
			TEENSY_CRASHLOG_BREADCRUMB(4, 0x0);
		};
		using Railway = _Railway;
		using Tracks = typename Railway::Tracks;

#ifdef WINSTON_STATISTICS
		const std::string statistics(const size_t withTop = 0) const { return this->stopWatchJournal.toString(withTop); }
		const std::string statisticsSignalTower(const size_t withTop = 0) const { return this->signalTower->statistics(withTop); }

	protected:
		StopwatchJournal stopWatchJournal;
#endif
		const bool isReady() const
		{
			return this->status() == Status::Ready;
		}
	private:
		void setupNextSignals() const
		{
			this->railway->eachTrack([this](const Tracks tracksId, winston::Track::Shared track) {
				track->eachConnection([this, track](Track& unused, const Track::Connection connection) {
					if (connection != winston::Track::Connection::DeadEnd)
					{
						SignalTower::setupNextSignal(track, connection, Signal::Pass::Facing);
						SignalTower::setupNextSignal(track, connection, Signal::Pass::Backside);
					}
				});
			});
		}

	protected:

		enum class Status : uint8_t
		{
			Initializing = 0,
			Ready,
			Faulty
		};
		Status _status;
		const Status status() const
		{
			return this->_status;
		}

		virtual void systemSetup() = 0;
		virtual void systemSetupComplete() = 0;
		virtual void systemLoop() { };
		virtual void populateSheds() = 0;

		virtual void setupSignals() = 0;

		virtual const winston::Result setupDetectors() = 0;

		void addLocomotive(const winston::Locomotive::Callbacks callbacks, const Address address, const winston::Locomotive::Functions functions, const Position start, const ThrottleSpeedMap speedMap, const std::string name, const Length length, const Locomotive::Types types)
		{
			auto loco = Locomotive::make(callbacks, address, functions, start, speedMap, name, length, types);
			this->locomotiveShed.add(loco);
			loco->autodrive(false, false, true, true);
			this->locomotiveShed.load(loco, [&](unsigned int trackIndex) {
				return this->railway->track(trackIndex);
				});
			
		}

		Result loadLocomotives()
		{
			return winston::Result::OK;
		}

		Result storeLocomotives()
		{
			return winston::Result::OK;
		}

		/*using SignalFactory = std::function < winston::Signal::Shared(winston::Track::Shared track, winston::Track::Connection connection)>;
		template<class _Signal>
		Signal::Shared signalFactory(winston::Track::Shared track, const winston::Track::Connection connection, winston::Distance distance, unsigned int& port, const winston::Railway::Callbacks::SignalUpdateCallback& signalUpdateCallback)
		{
			auto s = _Signal::make([track, connection, signalUpdateCallback](const winston::Signal::Aspects aspect)->const winston::State {
				return signalUpdateCallback(track, connection, aspect);
				}, distance, port);
			track->attachSignal(s, connection);
			port += (unsigned int)_Signal::lightsCount();
			return s;

			/* TODO: ensure port does not overflow
			return [distance, devPort, this](winston::Track::Shared track, winston::Track::Connection connection)->winston::Signal::Shared {
				return _Signal::make([=](const winston::Signal::Aspects aspect)->const winston::State {
					return this->callbacks.signalUpdateCallback(track, connection, aspect);
					}, distance, devPort);9
		}*/

		virtual Result detectorUpdate(winston::Detector::Shared detector, Locomotive &loco) = 0;

		// the railway
		typename Railway::Shared railway;

		SignalTower::Shared signalTower;

		// the z21 digital central station
		typename _DigitalCentralStation::Shared digitalCentralStation;
		typename _AddressTranslator::Shared addressTranslator;
		DigitalCentralStation::DebugInjector::Shared stationDebugInjector;

		// the locos
		LocomotiveShed locomotiveShed;

		// the rail cars
		RailCarShed railCarShed;
		static const RailCarShed getRailCarsFromShed(const RailCarShed &railCarShed, const RailCar::Groups::Group matching)
		{
			RailCarShed result;
			std::copy_if(railCarShed.begin(), railCarShed.end(), std::back_inserter(result),
				[&matching](const RailCarShed::value_type car) -> const bool
				{ 
					return car->is(matching); 
				}
			);

			return result;
		}

		TimePoint lastTurnoutToggleRequest{ winston::hal::now() };
		TimePoint lastLocoStatusRequest{ winston::hal::now() };
		TimePoint lastLocoWebUIPositionUpdate{ winston::hal::now() };

		Looper::Shared detectorDevice;

		std::map<Track::Shared, bool> checkedTurnoutsDuringInit;
	};
}

