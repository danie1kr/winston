#pragma once

#include <string>
#include <array>
#include <memory>
#include <vector>
#include <span>

namespace winston
{
	using Address = uint16_t;

	using Id = unsigned int;

	using TrackIndex = size_t;


	enum class State
	{
		Running,
		Skipped,
		Finished
	};

	enum class Result
	{
		OK,
		InternalError,
		SendFailed,
		ValidationFailed,
		ExternalHardwareFailed,
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

	template<typename T, unsigned int bits = 8 * sizeof(T)>
	class SendDevice : public Shared_Ptr<SendDevice<T, bits>>
	{
	public:
		using DataType = T;
		SendDevice() : skip(false) { };
		virtual const Result send(const std::span<DataType> data) = 0;
		void skipSend(bool skip)
		{
			this->skip = skip;
		}

		using Shared_Ptr<SendDevice<T, bits>>::Shared;
		using Shared_Ptr<SendDevice<T, bits>>::make;
	protected:
		bool skip;
	};

	class Railway;

	class SignalBox;

	class DigitalCentralStation;
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation, Features _features>
	class ModelRailwaySystem;

	class Block;
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
}
