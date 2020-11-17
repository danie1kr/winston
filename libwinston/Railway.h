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
		Railway();
		virtual ~Railway() = default;
		///void connect(DigitalCentralStationP<std::shared_ptr<Railway>> digitalCentralStation);
	protected:
		//DigitalCentralStationP<std::shared_ptr<Railway>> digitalCentralStation;
	//public:
		//virtual Section::Shared& section(size_t index) = 0;
	};

	template<typename _SectionsClass>
	class RailwayWithRails : public Railway
	{
	public:
		RailwayWithRails() : Railway(), sections() { };
		virtual ~RailwayWithRails() = default;

		using Sections = typename _SectionsClass;
		//using RailwayDefineCallback = std::function<Section::Shared(const Sections section)>;
		//using RailConnectCallback = std::function<void(std::array < Section::Shared, sectionsCount()>& sections)>;

		Result init()//RailwayDefineCallback define, RailConnectCallback connect)
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

		//virtual const std::string name() = 0;
		
		template<typename _Section, typename ..._args>
		Section::Shared& add(Sections section, _args && ...args) {
			return this->sections[static_cast<size_t>(section)] = std::make_shared<_Section>(std::forward<_args>(args)...);
		}


		//void drive(SectionIndex from, SectionIndex& onto) const;

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
			//std::bitset<_Sections> passed({});
			bool passed = true;

			for(size_t i = 0; i < sectionsCount() && passed; ++i)
			{
				auto current = this->sections[i];
				if (!current)
					return Result::ValidationFailed;

				passed = current->validate() == Result::Ok;
			}

			/*std::queue<Section*> rails;

			rails.push(sections[0]);
			while (rails.size() > 0)
			{
				auto current = rails.front();
				rails.pop();



				if (current == nullptr)
					return Result::ValidationFailed;
			}*/

			return passed ? Result::Ok : Result::InternalError;
		}
		//void leaving(SectionIndex section);
		//void entering(SectionIndex section);

	protected:
		std::vector<Locomotive> locomotives;
		std::array<Section::Shared, sectionsCount()> sections;
	};
}