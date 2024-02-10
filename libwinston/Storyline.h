#pragma once

#include <stack>
#include "WinstonTypes.h"
#include "Command.h"
#include "Locomotive.h"
#include "Section.h"
namespace winston
{
	class Storyline : public BasicCommand, public Shared_Ptr<Storyline>
	{
	public:

		Storyline();
		~Storyline() = default;
		using Shared_Ptr<Storyline>::Shared;
		using Shared_Ptr<Storyline>::make;
		using Shared_Ptr<Storyline>::enable_shared_from_this_virtual::shared_from_this;
		
		struct Context;
		
		class Task : public Shared_Ptr<Task>
		{
		public:
			Task();
			virtual ~Task() = default;
			using Shared_Ptr<Task>::Shared;
			using Shared_Ptr<Task>::make;
			using List = std::list<Task::Shared>;

			virtual const State execute(const Storyline::Shared storyline, const List& context) = 0;
			virtual const std::string text() const = 0;

			void reset();
			const bool valid() const;

		private:
			bool _valid;
		};

		struct Reply
		{
			enum class Answer : unsigned char
			{
				None, Refresh, Cancel
			};
			using Callback = std::function<const State(const Answer)>;
		};

		const State execute();

		using Invent = std::function<Task::List()>;
		using TextCallback = std::function<const std::string(const Task::List& context)>;

		void invent(const Invent inventCallback, const TextCallback textCallback);
		const std::string text() const;

		const Result reply(const Reply::Answer answer);
		const Result reply(const std::string& answer);

		template<typename T>
		static const typename T::Shared get(const Task::List& tasks)
		{
			for (auto& task : tasks)
			{
				auto t = std::dynamic_pointer_cast<T>(task);
				if (t)
					return t;
			}
			return nullptr;
		}

		template<typename T>
		const typename T::Shared getAhead() const
		{
			return this->get<T>(this->tasks);
		}

		template<typename T>
		const typename T::Shared getContext() const
		{
			return this->get<T>(this->context);
		}


	private:
		void reinvent();

		Task::List tasks;
		Task::List context;
		TextCallback textCallback;
		Invent inventCallback;
	};

	class TaskCallback : public Storyline::Task, public Shared_Ptr<TaskCallback>
	{
	public:
		using Callback = std::function<const State(const Storyline::Shared storyline, const Task::List& context)>;

		TaskCallback(const Callback callback);
		~TaskCallback() = default;
		using Shared_Ptr<TaskCallback>::Shared;
		using Shared_Ptr<TaskCallback>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;

	private:
		const Callback callback;
	};

	class TaskRandomCars : public Storyline::Task, public Shared_Ptr<TaskRandomCars>
	{
	public:
		TaskRandomCars(const RailCarShed& railCarShed);
		~TaskRandomCars() = default;
		using Shared_Ptr<TaskRandomCars>::Shared;
		using Shared_Ptr<TaskRandomCars>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;

		const std::list<RailCar::Shared> railCars() const;

	private:
		const RailCarShed& railCarShed;
		std::list<RailCar::Shared> _railCars;

		void findCars(const RailCar::Groups::Group group, const unsigned char probOneOnly, const unsigned char maxNumCars, const Length maxLength);
	};

	class TaskRandomAction : public Storyline::Task, public Shared_Ptr<TaskRandomAction>
	{
	public:
		enum class Action {
			Assemble,
			Drive,
			Stable
		};
		TaskRandomAction();
		~TaskRandomAction() = default;
		using Shared_Ptr<TaskRandomAction>::Shared;
		using Shared_Ptr<TaskRandomAction>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;

		const Action action() const;
	private:
		Action _action;
	};

	class TaskRandomLoco : public Storyline::Task, public Shared_Ptr<TaskRandomLoco>
	{
	public:
		TaskRandomLoco(const LocomotiveShed &locoShed);
		~TaskRandomLoco() = default;
		using Shared_Ptr<TaskRandomLoco>::Shared;
		using Shared_Ptr<TaskRandomLoco>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;

		const Locomotive::Shared locomotive() const;
	private:
		const LocomotiveShed& locoShed;
		Locomotive::Shared _locomotive;
	};

	class TaskRandomSection : public Storyline::Task, public Shared_Ptr<TaskRandomSection>
	{
	public:
		TaskRandomSection(const SectionList &list);
		~TaskRandomSection() = default;
		using Shared_Ptr<TaskRandomSection>::Shared;
		using Shared_Ptr<TaskRandomSection>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;

		const Section::Shared section() const;
	private:
		const SectionList list;
		Section::Shared _section;
	};

	class TaskReply : public Storyline::Task, Shared_Ptr<TaskReply>
	{
	public:
		TaskReply(const Storyline::Reply::Callback callback);
		virtual ~TaskReply() = default;
		using Shared_Ptr<TaskReply>::Shared;
		using Shared_Ptr<TaskReply>::make;

		const State execute(const Storyline::Shared storyline, const Task::List& context);
		const std::string text() const;
		void reply(const Storyline::Reply::Answer answer);
	private:
		const Storyline::Reply::Callback callback;
		Storyline::Reply::Answer lastAnswer;
	};

	/*
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
		TaskRandomTrack(const Section::Type type);
		virtual ~TaskRandomTrack() = default;
		const State execute();
		const std::string text() const;

		using Shared_Ptr<TaskRandomTrack>::Shared;
		using Shared_Ptr<TaskRandomTrack>::make;
	private:
		const Section::Type type;
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

		*

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

