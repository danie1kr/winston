#include "Signal.h"

namespace winston
{
	Signal::Signal(Callback callback)
		: callback(callback), aspect(Aspect::Stop)
	{

	}

	Task::State Signal::set(Aspect aspect)
	{
		return this->callback(aspect);
	}

	Signal::Aspect Signal::state()
	{
		return this->aspect;
	}

/*
	PreSignal::PreSignal(Device& device, const Port stop, const Port go)
		: Signal(device, stop, go)
	{

	}

	SectionSignal::SectionSignal(Device& device, const Port stop, const Port go)
		: Signal(device, stop, go)
	{

	}

	SectionWithPreSignal::SectionWithPreSignal(Device& device, const Port stop, const Port go, PreSignal& preSignal)
		: SectionSignal(device, stop, go), preSignal(preSignal)
	{

	}

	void SectionWithPreSignal::stop()
	{
		SectionSignal::stop();
		preSignal.stop();
	}

	void SectionWithPreSignal::go()
	{
		SectionSignal::go();
		preSignal.go();
	}
	*/
}