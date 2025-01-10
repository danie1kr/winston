#pragma once

#include "Signal.h"
#include "Track.h"

namespace winston
{
	struct NextSignal : public Shared_Ptr<NextSignal>
	{
		const Signal::Shared signal;
		const Track::Const track;
		const Track::Connection connection;
		const Distance distance;
		const Signal::Pass pass;

		using Shared_Ptr<NextSignal>::Shared;
		using Shared_Ptr<NextSignal>::Const;
		using Shared_Ptr<NextSignal>::make;

		NextSignal(const Signal::Shared signal, const Track::Const track, const Track::Connection connection, const Distance distance, const Signal::Pass pass);
		~NextSignal() = default;
	};

	struct NextSignals
	{
		NextSignals();
		~NextSignals() = default;
		void put(NextSignal::Const next, const bool forward, const Signal::Pass pass);
		const NextSignal::Const get(const bool forward, const Signal::Pass pass) const;
		const bool contains(const Signal::Const signal) const;
	private:
		static constexpr size_t index(const bool forward, const Signal::Pass pass);
		std::array<NextSignal::Const, 4> nextSignals;
	};
}