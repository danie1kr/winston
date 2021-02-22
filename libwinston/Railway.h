#pragma once

#include <vector>
#include <array>
#include <algorithm>
#include <bitset>
#include <memory>
#include <set>
#include <queue>

#include "magic_enum.hpp"

#include "WinstonTypes.h"
#include "Util.h"
#include "Rail.h"

namespace winston
{
	class Railway : public Shared_Ptr<Railway>
	{
	public:
		struct Callbacks
		{
			using TurnoutUpdateCallback = std::function<const State(Turnout::Shared turnout, const Turnout::Direction direction)>;
			TurnoutUpdateCallback turnoutUpdateCallback;

			using SignalUpdateCallback = std::function<const State(Signal::Shared signal, const Signal::Aspect aspect)>;
			SignalUpdateCallback signalUpdateCallback;
		};

		Railway(const Callbacks callbacks);
		virtual ~Railway() = default;
	protected:
		const Callbacks callbacks;
	};

	template<typename _SectionsClass>
	class RailwayWithRails : public Railway
	{
	public:
		RailwayWithRails(const Callbacks callbacks) : Railway(callbacks), sections() { };
		virtual ~RailwayWithRails() = default;

		using Sections = _SectionsClass;

		Result init()
		{
			for (size_t section = 0; section < sectionsCount(); ++section)
				this->sections[section] = define(magic_enum::enum_value<Sections>(section));
			connect(this->sections);
			return this->validate();
		}
		static constexpr size_t sectionsCount() noexcept {
			return magic_enum::enum_count<Sections>();
		}

		virtual winston::Section::Shared define(const Sections section) = 0;
		virtual void connect(std::array < winston::Section::Shared, sectionsCount()>& sections) = 0;

		template<typename _Section, typename ..._args>
		Section::Shared& add(Sections section, _args && ...args) {
			return this->sections[static_cast<size_t>(section)] = std::make_shared<_Section>(std::forward<_args>(args)...);
		}

		bool traverse(const Section::Connection from, Section::Shared& on, Section::Shared& onto) const
		{
			return on ? on->traverse(from, onto) : false;
		}

		inline Sections sectionEnum(size_t index)
		{
			return magic_enum::enum_cast<Sections>((unsigned int)index).value();
		}

		inline unsigned int sectionIndex(Sections section)
		{
			return magic_enum::enum_integer(section);
		}

		inline unsigned int sectionIndex(Section::Shared section)
		{
			return magic_enum::enum_integer(sectionEnum(section));
		}

		inline Section::Shared& section(Sections index)
		{
			return this->sections[static_cast<size_t>(index)];
		}

		inline Section::Shared& section(size_t index)
		{
			return this->sections[index];
		}

		Sections sectionEnum(Section::Shared& section)
		{
			auto it = std::find(this->sections.begin(), this->sections.end(), section);
			return this->sectionEnum(std::distance(this->sections.begin(), it));
		}

		inline Sections section(Bumper::Shared& section)
		{
			auto s = std::dynamic_pointer_cast<Section>(section);
			return this->section(s);
		}
		
		inline Sections section(Rail::Shared& section)
		{
			auto s = std::dynamic_pointer_cast<Section>(section);
			return this->section(s);
		}

		inline Sections section(Turnout::Shared& section)
		{
			auto s = std::dynamic_pointer_cast<Section>(section);
			return this->section(s);
		}
	private:
		Result validate()
		{
			bool passed = true;

			for(size_t i = 0; i < sectionsCount() && passed; ++i)
			{
				auto current = this->sections[i];
				if (!current)
					return Result::ValidationFailed;

				passed = current->validate() == Result::OK;
			}

			return passed ? Result::OK : Result::InternalError;
		}

	protected:
		std::array<Section::Shared, sectionsCount()> sections;
	};
}
