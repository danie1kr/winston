#pragma once

#include <string>
#include <array>
#include <memory>

namespace winston
{
	using DCC_Address = int;

	using Id = unsigned int;

	using SectionIndex = size_t;

	struct Locomotive
	{
		DCC_Address dcc;
		std::string name;
	};

	enum class Result
	{
		Ok,
		InternalError,
		ValidationFailed
	};


	template<class _T>
	class Shared_Ptr //: public std::enable_shared_from_this<_T>
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
	//using RailwayP = std::shared_ptr<Railway>;

	class SignalBox;
	//using SignalBoxP = std::shared_ptr<SignalBox>;

	class DigitalCentralStation;
	template<typename _Railway, class _AddressTranslator, class _DigitalCentralStation>
	class ModelRailwaySystem;
	//using DigitalCentralStationP = std::shared_ptr<DigitalCentralStation>;

	class Block;
	//class Signal;
	//enum class Signal::Aspect;
	class Section;
	//using SectionP = std::shared_ptr <Section>;
	class Bumper;
	//using BumperP = std::shared_ptr <Bumper>;
	class Rail;
	//using RailP = std::shared_ptr <Rail>;
	class Turnout;
	//using TurnoutP = std::shared_ptr <Turnout>;
	//enum class Turnout::Direction;

	class Task;
	//using TaskP = std::unique_ptr <Task>;
	//enum class Task::State;
	
	class Event;
	//using EventP = std::unique_ptr <Event>;

	class Callback;
	//using CallbackP = std::shared_ptr<Callback>;

}