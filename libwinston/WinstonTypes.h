#pragma once

#ifdef __GNUC__ 
#pragma GCC push_options
#pragma GCC optimize("Os")
#endif
#include "better_enum.hpp"
#include <string>
#include <array>
#include <vector>
#include "span.hpp"
#ifdef __GNUC__ 
#pragma GCC pop_options
#endif
#include <memory>

//#ifdef WINSTON_PLATFORM_WIN_x64
//#define WINSTON_HAS_CHRONO
#include <chrono>
//#endif

namespace winston
{
	using Address = uint16_t;
	using NFCAddress = unsigned long long;

	using Id = unsigned int;

	using TrackIndex = size_t;


	enum class State
	{
		Running,
		Skipped,
		Delay,
		Finished
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
	};

	enum class Features : unsigned int
	{
		None = 0,
		Blocks = 1
	};

	inline Features operator|(Features a, Features b)
	{
		return static_cast<Features>(static_cast<int>(a) | static_cast<int>(b));
	}

	inline bool operator&(Features a, Features b)
	{
		return static_cast<bool>(static_cast<int>(a) & static_cast<int>(b));
	}

	class Port
	{
	public:
		Port();
		Port(const size_t device, const size_t port);

		const size_t device() const;
		const size_t port() const;
	private:
		const size_t _device;
		const size_t _port;
	};

	template<class _T>
	class Shared_Ptr
	{
	public:
		using Shared = std::shared_ptr<_T>;

		template<typename... Args>
		static Shared make(Args&&... args) {
			return std::make_shared<_T>(std::forward<Args>(args)...);
		}
	};

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
#define RUNTIME_LAMBDAS(what) \
	auto runtime##what = []() { return runtimeHardwareState.enabled(RuntimeHardwareState::Type::what); }; \
	auto runtimeEnable##what = []() { runtimeHardwareState.enable(RuntimeHardwareState::Type::what); };

	RUNTIME_LAMBDAS(Serial);
	RUNTIME_LAMBDAS(SPI);
	RUNTIME_LAMBDAS(Persistence);
	RUNTIME_LAMBDAS(Network);
	RUNTIME_LAMBDAS(Railway);
	void logRuntimeStatus();

	template<size_t>
	class Log;

	class Railway;

	class SignalBox;

	class DigitalCentralStation;
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation, Features _features>
	class ModelRailwaySystem;

	class Block;
	class Detector;
	template<typename T> class DetectorAddressable;
	using DCCDetector = DetectorAddressable<Address>;
	using NFCDetector = DetectorAddressable<NFCAddress>;
	class Track;
	class Bumper;
	class Rail;
	class Turnout;
	class Task;

	class Signal;
	
	class Event;

	class Callback;

	class UDPSocket;

	class Locomotive;

	class Command;
	class Payload;

	using Length = unsigned int;
	using Distance = int;

	using TimePoint = std::chrono::system_clock::time_point;
	using Duration = TimePoint::duration;
#define toSeconds(x) (std::chrono::seconds(x))
#define toMilliseconds(x) (std::chrono::milliseconds(x))
#define toMicroseconds(x) (std::chrono::microseconds(x))
#define inSeconds(x) ((x) / toSeconds(1))
#define inMilliseconds(x) ((x) / toMilliseconds(1))
#define inMicroseconds(x) ((x) / toMicroseconds(1))	/*
#ifdef WINSTON_PLATFORM_WIN_x64
	using TimePoint = std::chrono::system_clock::time_point;
	using Duration = TimePoint::duration;

#define inTheFuture(now, add) now + add
#elif defined(WINSTON_PLATFORM_TEENSY)
	using TimePoint = unsigned long long;
	using Duration = unsigned long long;
#define toSeconds(x) (x/10000000)
#define toMilliseconds(x) (x/1000)
#define toMicroseconds(x) (x)
#define inSeconds(x) (toSeconds(x))
#define inMilliseconds(x) (toMilliseconds(x))
#define inMicroseconds(x) (toMicroseconds(x))
#endif
*/
}
