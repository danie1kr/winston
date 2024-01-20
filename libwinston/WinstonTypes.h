#pragma once
#ifndef WINSTON_TYPES_H
#define WINSTON_TYPES_H

#ifdef __GNUC__ 
#pragma GCC push_options
//#pragma GCC optimize("Os")
#endif
#include <string>
#include <array>
#include <vector>
#include "../libwinston/external/better_enum.hpp"
#ifdef __GNUC__ 
#pragma GCC pop_options
#endif
#include <memory>

#ifndef WINSTON_PLATFORM_ESP32
#include <chrono>
using namespace std::chrono_literals;
#endif

#include "WinstonSharedTypes.h"

namespace winston
{
	using Id = unsigned int;
	using TrackIndex = size_t;

	enum class State
	{
		New,
		Running,
		Skipped,
		Delay,
		Finished,
		Aborted
	};

	enum class Result
	{
		OK,
		InternalError,
		SendFailed,
		ReceiveFailed,
		ValidationFailed,
		ExternalHardwareFailed,
		NotInitialized,
		NotFound,
		OutOfBounds,
		InvalidParameter,
		OutOfMemory,
		NotImplemented
	};

	enum class Features : unsigned int
	{
		None =		0b000,
		Sections =	0b001,
		Detectors = 0b010,
		Routes	  = 0b100
	};

	inline Features operator|(Features a, Features b)
	{
		return static_cast<Features>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline bool operator&(Features a, Features b)
	{
		return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
	}
/*
	class Port
	{
	public:
		Port(const size_t device, const size_t port);

		const size_t device() const;
		const size_t port() const;
	private:
		const size_t _device;
		const size_t _port;
	};*/
	using Port = unsigned int;

	// from https://stackoverflow.com/a/14941915
	template<typename T>
	struct enable_shared_from_this_virtual;

	class enable_shared_from_this_virtual_base : public std::enable_shared_from_this<enable_shared_from_this_virtual_base>
	{
		typedef std::enable_shared_from_this<enable_shared_from_this_virtual_base> base_type;
		template<typename T>
		friend struct enable_shared_from_this_virtual;

		std::shared_ptr<enable_shared_from_this_virtual_base> shared_from_this()
		{
			return base_type::shared_from_this();
		}
		std::shared_ptr<enable_shared_from_this_virtual_base const> const_from_this() const
		{
			return base_type::shared_from_this();
		}
	};

	template<typename T>
	struct enable_shared_from_this_virtual : virtual enable_shared_from_this_virtual_base
	{
		typedef enable_shared_from_this_virtual_base base_type;

	public:
		std::shared_ptr<T> shared_from_this()
		{
			std::shared_ptr<T> result(base_type::shared_from_this(), static_cast<T*>(this));
			return result;
		}

		std::shared_ptr<T const> const_from_this() const
		{
			std::shared_ptr<T const> result(base_type::const_from_this(), static_cast<T const*>(this));
			return result;
		}
	};

	template<class _T>
	class Shared_Ptr : public enable_shared_from_this_virtual<_T>
	{
	public:
		using Shared = std::shared_ptr<_T>;
		using Const = std::shared_ptr<_T const>;
		template<typename... Args>
		[[nodiscard]] static Shared make(Args&&... args) {
			return std::make_shared<_T>(std::forward<Args>(args)...);
		}
	};

#ifndef WINSTON_PLATFORM_ESP32
	template<class _T>
	class Uniqe_Ptr
	{
	public:
		using Unique = std::unique_ptr<_T>;

		template<typename... Args>
		static Unique make(Args&&... args) {
			return std::make_unique<_T>(std::forward<Args>(args)...);
		}
	};
#endif

	class Device : public Shared_Ptr<Device>
	{
	public:
		virtual const Result init() = 0;
		using Shared_Ptr<Device>::Shared;
		using Shared_Ptr<Device>::make;
	};

	template<typename T>
	class ReadDevice : public Shared_Ptr<ReadDevice<T>>
	{
	public:
		using DataType = T;
		ReadDevice() { };
		virtual const size_t available() = 0;
		virtual const T read() = 0;
		virtual const size_t read(std::vector<T>& content, size_t upTo) = 0;

		using Shared_Ptr<ReadDevice<T>>::Shared;
		using Shared_Ptr<ReadDevice<T>>::make;
	};

	template<typename T>
	class SendDevice : public Shared_Ptr<SendDevice<T>>
	{
	public:
		using DataType = T;
		SendDevice() : skip(false) { };
		virtual const Result send(const std::vector<DataType> data) = 0;
		void skipSend(bool skip)
		{
			this->skip = skip;
		}

		using Shared_Ptr<SendDevice<T>>::Shared;
		using Shared_Ptr<SendDevice<T>>::make;
	protected:
		bool skip;
	};

	class GPIOPinDevice : public Shared_Ptr<GPIOPinDevice>
	{
	public:
		enum class State : unsigned char
		{
			Low = 0,
			High = 255
		};

		using Pin = unsigned char;

		GPIOPinDevice(const Pin pin);
	protected:
		const Pin pin;
	};

	class GPIODigitalPinOutputDevice : public GPIOPinDevice, public Shared_Ptr<GPIODigitalPinOutputDevice>
	{
	public:
		GPIODigitalPinOutputDevice(const Pin pin, const State initial = State::Low);
		virtual void set(const State value) = 0;

		using Shared_Ptr<GPIODigitalPinOutputDevice>::Shared;
		using Shared_Ptr<GPIODigitalPinOutputDevice>::make;
	};

	class GPIODigitalPinInputDevice : public GPIOPinDevice, public Shared_Ptr<GPIODigitalPinInputDevice>
	{
	public:
		enum class Mode : unsigned char
		{
			Input,
			InputPullup
		};

		GPIODigitalPinInputDevice(const Pin pin, const Mode mode = Mode::Input);
		virtual const State read() = 0;

		using Shared_Ptr<GPIODigitalPinInputDevice>::Shared;
		using Shared_Ptr<GPIODigitalPinInputDevice>::make;
	};

	class GPIODevice : public Shared_Ptr<GPIODevice>
	{
	public:
		virtual GPIODigitalPinInputDevice::Shared getInputPinDevice(GPIODigitalPinInputDevice::Pin pin, GPIODigitalPinInputDevice::Mode) = 0;
		virtual GPIODigitalPinOutputDevice::Shared getOutputPinDevice(GPIODigitalPinOutputDevice::Pin pin) = 0;
	};

	class RuntimeHardwareState
	{
	private:
		using StateType = unsigned char;
	public:
		enum class Type : StateType
		{
			Railway			= 1,
			Persistence		= 2,
			Network			= 3,
			SPI				= 4,
			Serial			= 5
		};
		RuntimeHardwareState();

		inline void enable(const Type what)
		{
			this->state |= 1 << (RuntimeHardwareState::StateType)what;
		}
		inline void disable(const Type what)
		{
			this->state &= ~(1UL << (RuntimeHardwareState::StateType)what);
		}
		inline bool enabled(const Type what) const
		{
			return (this->state >> (RuntimeHardwareState::StateType)what) & 1U;
		}
	private:
		StateType state;
	};
	extern RuntimeHardwareState runtimeHardwareState;


#define DEFINE_RUNTIME_LAMBDA(what) \
	bool runtime##what(); \
	void runtimeEnable##what();

	DEFINE_RUNTIME_LAMBDA(Serial);
	DEFINE_RUNTIME_LAMBDA(SPI);
	DEFINE_RUNTIME_LAMBDA(Persistence);
	DEFINE_RUNTIME_LAMBDA(Network);
	DEFINE_RUNTIME_LAMBDA(Railway);

	/*
#define RUNTIME_LAMBDAS(what) \
	auto runtime##what = []() { return runtimeHardwareState.enabled(RuntimeHardwareState::Type::what); }; \
	auto runtimeEnable##what = []() { runtimeHardwareState.enable(RuntimeHardwareState::Type::what); };

	RUNTIME_LAMBDAS(Serial);
	RUNTIME_LAMBDAS(SPI);
	RUNTIME_LAMBDAS(Persistence);
	RUNTIME_LAMBDAS(Network);
	RUNTIME_LAMBDAS(Railway);*/
	void logRuntimeStatus();

	template<size_t>
	class Log;

	class Railway;
	BETTER_ENUM(RoutesNone, unsigned int, None);
	BETTER_ENUM(SectionsSingle, unsigned int, Single);

	class SignalTower;

	class DigitalCentralStation;
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation, class _WebServer>
	class ModelRailwaySystem;

	class Section;
	class Detector;
	template<typename T> class DetectorAddressable;
	using DCCDetector = DetectorAddressable<Address>;
	class Track;
	class Bumper;
	class Rail;
	class Turnout;
	class Task;

	class Signal;
	class SignalDevice;
	class SignalController;
	
	class Event;

	class UDPSocket;

	class Locomotive;

	class BasicCommand;
	class Command;
	class Payload;

	using Length = unsigned int;
	using Distance = int;

#ifndef WINSTON_PLATFORM_ESP32
	using TimePoint = std::chrono::system_clock::time_point;
	using Duration = TimePoint::duration;
#define toSeconds(x) (std::chrono::seconds(x))
#define toMilliseconds(x) (std::chrono::milliseconds(x))
#define toMicroseconds(x) (std::chrono::microseconds(x))
#define inSeconds(x) ((x) / toSeconds(1))
#define inMilliseconds(x) ((x) / toMilliseconds(1))
#define inMicroseconds(x) ((x) / toMicroseconds(1))
#else
	using TimePoint = uint32_t;
	using Duration = uint32_t;
#define toSeconds(x) (x / 1000)
#define toMilliseconds(x) (x)
#define toMicroseconds(x) (x * 1000)
#define inSeconds(x) ((x) / toSeconds(1))
#define inMilliseconds(x) ((x) / toMilliseconds(1))
#define inMicroseconds(x) ((x) / toMicroseconds(1))
#endif
}
#endif