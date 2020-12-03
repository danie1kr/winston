#pragma once

#include <string>
#include <array>
#include <memory>

namespace winston
{
	using Address = uint16_t;

	using Id = unsigned int;

	using SectionIndex = size_t;


	enum class State
	{
		Running,
		Finished
	};

	enum class Result
	{
		OK,
		InternalError,
		SendFailed,
		ValidationFailed
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

	class Railway;

	class SignalBox;

	class DigitalCentralStation;
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation>
	class ModelRailwaySystem;

	class Block;
	class Section;
	class Bumper;
	class Rail;
	class Turnout;
	class Task;
	
	class Event;

	class Callback;

	class UDPSocket;

	class Locomotive;

	class Command;
	class Payload;
}
