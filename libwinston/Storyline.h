#pragma once

#include <stack>
#include "WinstonTypes.h"
#include "Command.h"
#include "Locomotive.h"
#include "Block.h"
namespace winston {
	class Storyline : public BasicCommand, public Shared_Ptr<Storyline>
	{
	public:
		using Shared_Ptr<Storyline>::Shared;
		using Shared_Ptr<Storyline>::make;

		class Task : public Shared_Ptr<Task>
		{
		public:
			Task();
			virtual ~Task() = default;

			virtual const State execute() = 0;
			virtual const std::string text() const = 0;

			const State abort() const;
		};

		using Invent = std::function<void()>;

		Storyline();
		~Storyline() = default;

		const State execute();

		void queue(Task::Shared task);
		void immediate(Task::Shared task);

		const void invent(const Invent invent) const;

	private:
		std::deque<Task::Shared> tasks;
	};

	class TaskRandomLoco : public Storyline::Task, public Shared_Ptr<TaskRandomLoco>
	{
	public:
		TaskRandomLoco(const Locomotive::Types types);
		virtual ~TaskRandomLoco() = default;
		const State execute();
		const std::string text() const;

		using Shared_Ptr<TaskRandomLoco>::Shared;
		using Shared_Ptr<TaskRandomLoco>::make;
	private:
		const Locomotive::Types types;
	};

	class TaskRandomTrack : public Storyline::Task, public Shared_Ptr<TaskRandomTrack>
	{
	public:
		TaskRandomTrack(const Block::Type type);
		virtual ~TaskRandomTrack() = default;
		const State execute();
		const std::string text() const;

		using Shared_Ptr<TaskRandomTrack>::Shared;
		using Shared_Ptr<TaskRandomTrack>::make;
	private:
		const Block::Type type;
	};

	class ConfirmationProvider : public Shared_Ptr<ConfirmationProvider>
	{
	public:
		enum class Answer : unsigned char
		{
			None, Yes, No, Maybe
		};
		using Callback = std::function<void(const Answer)>;

		ConfirmationProvider();
		virtual ~ConfirmationProvider() = default;

		void setCallback(Callback callback);
		void confirm(const Answer answer);
		void clearCallbackTask();
	private:
		Callback callback;
	};

	class TaskConfirm : public Storyline::Task, public Shared_Ptr<TaskConfirm>
	{
	public:
		using MaybeCallback = std::function<State()>;
		using Display = SendDevice<std::string>;

		TaskConfirm(ConfirmationProvider::Shared confirmationProvider, Display::Shared display, const Task::Shared a, const std::string question, const Task::Shared b, const MaybeCallback maybe);
		virtual ~TaskConfirm() = default;
		const State execute();
		void confirm(const ConfirmationProvider::Answer answer);
		const std::string text() const;

		using Shared_Ptr<TaskConfirm>::Shared;
		using Shared_Ptr<TaskConfirm>::make;
	private:
		bool registered = false;
		ConfirmationProvider::Shared confirmationProvider;
		Display::Shared display;
		const Task::Shared a;
		const std::string question;
		const Task::Shared b;
		const MaybeCallback maybe;
		ConfirmationProvider::Answer answer = ConfirmationProvider::Answer::None;
	};

	class DisplayLog : public winston::TaskConfirm::Display, public winston::Shared_Ptr<DisplayLog>
	{
	public:
		DisplayLog();
		virtual ~DisplayLog() = default;
		virtual const winston::Result send(const std::vector<DataType> data);
		using winston::Shared_Ptr<DisplayLog>::Shared;
		using winston::Shared_Ptr<DisplayLog>::make;
	};

		/*

		std::function<State()> abort = [](std::string message) -> State { return State::Aborted; };
		
		loco = new LocoRandomizer(LOCO_PASSENGER | LOCO_GOODS, abort);
		assembleTrack = new TrackChooser(TRACK_?);

		confirmationStart = new Confirmation(loco, "auf", track, 
			// maybe
			[]() -> State
			{ 
				tasks.push_front(loco);
				tasks.push_front(track);
				return State::Running;
			});

		gotoAssembleTrack = new Drive(loco, assembletrack);

		wagons = new WagonChooser(LOCO_PASSENGER | LOCO_GOODS);
		confirmationAssemble = new Confirmation(wagons, "an", loco, "auf", track);

		nextTrack = new TrackChooser(TRACK_);
		drive = new DriveLaps(loco, nextTrack);

		MainStoryStates
			SETUP
			RUNNING
			DONE
		mainStory = new InlineTask<MainStoryStates>(SETUP, [](InlineTask<MainStoryStates> &self){
			if(self.state == SETUP)
			{

			}
			else if(self.state == RUNNING)
			{
			}
			else if(self.state == DONE)
			});

		drive = new Dri

		{
			{ INIT_LOCO, { loco, INIT_ASSEMBLE_TRACK } },
			{ INIT_ASSEMBLE_TRACK, { new TrackChooser(TRACK_?), CONFIRM_LOCO_TRACK } },
			{ CONFIRM_LOCO_TRACK, { new Confirmation(INIT_LOCO, "auf", INIT_ASSEMBLE_TRACK), DRIVE_ASSEMBLE_TRACK } },
			{ DRIVE_ASSEMBLE_TRACK, { new Drive(INIT_LOCO, INIT_ASSEMBLE_TRACK}
		}


		std::queue<Task> tasks;

		void tick()
		{
			task = this->tasks.front();
			if(task.execute() == Finished)
				this->tasks.pop_front();
		}

		class Task()
		{
			Task(Storyline storyline)
			{
				storyline.add(this);
			}

			virtual const State execute() = 0;
			virtual const std::string text() const = 0;

			const State abort() const
			{
				return State::Aborted;
			}
		}

		template<typename T>
		class InlineTask() : public Task
		{
			using Fn = std::function<T&()>;
			InlineTask(Fn fn);

			T& get();

			const State execute()
			{
				this->
			}

			private:
				T _t;
		}

		class LocoRandomizer()
		{
			LocoRandomizer(Storyline storyline)
			{
				
			}

			const State execute()
			{
				//choose loco
			}

			Locomotive get()
			{
				return loco;
			}

			private: 
				Locomotive loco;
		}
		
		loco = new LocoRandomizer(LOCO_PASSENGER | LOCO_GOODS);
		assembleTrack = new TrackChooser(TRACK_?);

		confirmationStart = new Confirmation(loco, "auf", track);
		
		gotoAssembleTrack = new Drive(loco, assembletrack);

		wagons = new WagonChooser(LOCO_PASSENGER | LOCO_GOODS);
		confirmationAssemble = new Confirmation(wagons, "an", loco, "auf", track);

		nextTrack = new TrackChooser(TRACK_);
		drive = new Drive(loco, nextTrack);

		mainStory = new Multiple(min, max, drive);

		drive = new Dri

		**********************
		
		void tick()
		{
			this->stack.top()->execute();
		}

		class Task
		{
			Task(MRS mrs) : state{New}
			setup() = 0
			
			start() {
				if(state == New)
				{
					stack.add(this);
					setup();
					state = Running;
				}
			}
			run() = 0
			execute()
			{
				if(state == New)
					start();
				else if(state == Running)
					run();
				else
					teardown();
			}
			teardown() = 0
			finish() {
				teardown();
				stack.pop();
			}

			State state;
		}

		class LocoRandomizer
		{
			void setup() {
				// get loco with light=off
				this->teardown();
			}
			Loco get() { if(this->state == NEW) this->setup(); return loco; }
			Loco loco;
		}

		class TrackRandomizer
		{
			void setup() {
				// get track with annotation
				this->teardown();
			}
			Track get() { if(this->state == NEW) this->setup(); return track; }
			Track track;
		}

		std::stack<Task> stack;

		*/
};

