// Copyright 2016, Jason Conaway
// LeanHsm - The flexible and efficient hierachical state machine
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <cstdarg>

#define LEAN_HSM_ALIASES(OwnerType, EventType) \
	using Hsm = LeanHsm::StateMachine<OwnerType, EventType>; \
	using State = Hsm::State; \
	using Name = Hsm::Name; \
	using StartIn = Hsm::StartIn; \
	using When = Hsm::When;

namespace LeanHsm
{
	template<typename OwnerType, typename EventType>
	class StateMachine
	{
	public:
		struct State;
		struct Transition;
		struct StartIn;
		struct When;

		using Event = EventType;
		using Action = std::function<void(StateMachine& owner)>;
		using Transitions = std::vector<Transition>;
		using Guard = std::function<bool(StateMachine& owner)>;
		using EventToString = std::function<std::string (EventType e)>;
		using Log = std::function<void(const char* format, va_list args)>;

		struct State
		{
			State Parent(const State& s) && { parent = &s; return std::move(*this);	}
			State OnEntry(const Action& a) && { entry = a; return std::move(*this); }
			State OnExit(const Action& a) && { exit = a; return std::move(*this); }
			State Initially(StartIn&& t) && { initialTransition = std::move(t);	return std::move(*this); }
			State Always(When&& t) && { transitions.emplace_back(std::forward<Transition>(t)); return std::move(*this); }
			
			const char* name{ nullptr };
			const State* parent{ nullptr };
			Action entry{ nullptr };
			Action exit{ nullptr };
			StartIn initialTransition;
			Transitions transitions;

			State(State&&) = default;
			State& operator=(State&&) = default;
		protected:
			explicit State(const char* n) : name(n) {} // For the Name type
		};

		// State definitions should begin with a named state
		struct Name : public State
		{
			explicit Name(const char* n) : State(n) {}
		};

		struct Transition
		{
			EventType eventId{};
			const State* target{ nullptr };
			Action action{ nullptr };

			Transition(Transition&&) = default;
			Transition& operator=(Transition&&) = default;
			Transition() = default;  // for default StartIn transition type
		protected:
			explicit Transition(const State& s) : target(&s) {} // for the StartIn transition type
			explicit Transition(const EventType& e) : eventId(e) {} // for the When transition type
		};

		struct StartIn : public Transition
		{
			StartIn() = default;
			explicit StartIn(const State& s) : Transition(s) {}
			StartIn Do(const Action& a) && { action = a; return std::move(*this); }
		};

		struct When : public Transition
		{
			explicit When(const EventType& e) : Transition(e) {}
			When Goto(const State& s) && { target = &s; return std::move(*this); }
			When Do(const Action& a) && { action = a; return std::move(*this); }
		};

		// StateMachine methods

		StateMachine(OwnerType& owner, const State& topState, const Log& log, const EventToString& e2s)
			: mOwner(owner), mCurrentState(&topState), mLog(log), mEventToString(e2s) {}
		void OnEntryAndExit(Action entry, Action exit) { mOnEntry = entry; mOnExit = exit; }
		void Initialize() { if (mCurrentState) { DoTransition(mCurrentState->initialTransition); } }
		OwnerType& GetOwner() const { return mOwner; }
		const State& CurrentState() const { return *mCurrentState; }
		bool IsInState(const State& s) const;
		bool HandeleEvent(const EventType& e);

	private:
		std::vector<const State*> GetCommonAncestorPath(const State* a, const State* b) const;
		bool DoTransition(const Transition& t);
		enum Severity { Info, Warning, Error };
		void LogEntry(Severity severity, const char* format, ...);

		OwnerType& mOwner;
		const State* mCurrentState{ nullptr };
		Action mOnEntry;
		Action mOnExit;
		Log mLog;
		EventToString mEventToString;
	};

	template<typename OwnerType, typename EventType>
	std::vector<typename const StateMachine<OwnerType, EventType>::State*>
		StateMachine<OwnerType, EventType>::GetCommonAncestorPath(const State* a, const State* b) const
	{
		std::vector<const State*> aPath;
		std::vector<const State*> bPath;
		while (a)
		{
			aPath.push_back(a);
			a = a->parent;
		}
		while (b)
		{
			bPath.push_back(b);
			b = b->parent;
		}
		const State* ancestor{ nullptr };
		while (!aPath.empty() && !bPath.empty() && aPath.back() == bPath.back())
		{
			ancestor = bPath.back();
			aPath.pop_back();
			bPath.pop_back();
		}
		if (ancestor)
		{
			bPath.push_back(ancestor);
		}
		return std::move(bPath);
	}

	template<typename OwnerType, typename EventType>
	bool StateMachine<OwnerType, EventType>::HandeleEvent(const EventType& e)
	{
		if (!mCurrentState)
		{
			LogEntry(Error, "Cannot transition from a null state");
			return false; 
		}
		auto state = mCurrentState;
		while (state)
		{
			// find transition
			auto transition = std::find_if(
				begin(state->transitions),
				end(state->transitions),
				[e](const Transition& t) { return t.eventId == e; });
			if (transition != end(state->transitions))
			{				
				LogEntry(Info, "Event [%s]", mEventToString(e).c_str());
				DoTransition(*transition);
				return true;
			}
			else
			{
				state = state->parent;
			}
		}

		LogEntry(Warning, "No transition for event [%s] from %s",
			mEventToString(e).c_str(), mCurrentState->name);
		return false;
	}

	template<typename OwnerType, typename EventType>
	bool StateMachine<OwnerType, EventType>::DoTransition(const Transition& transition)
	{
		if (!mCurrentState)
		{
			LogEntry(Error, "Cannot transition from a null state");
			return false;
		}

		auto target = transition.target;
		if (!target) 
		{
			target = mCurrentState;
		}
		LogEntry(Info, "%s -> %s", mCurrentState->name, target->name);

		// exit up to common ancestor
		auto targetPath = GetCommonAncestorPath(mCurrentState, target);
		if (!targetPath.empty())
		{
			auto ancestor = targetPath.back();
			targetPath.pop_back();
			while (mCurrentState != ancestor)
			{
				if (mOnExit)
				{
					mOnExit(*this);
				}
				if (mCurrentState->exit)
				{
					mCurrentState->exit(*this);
				}
				if (mCurrentState->parent)
				{
					mCurrentState = mCurrentState->parent;
				}
			}
		}		

		// do transition action
		if (transition.action)
		{
			transition.action(*this);
		}

		bool wasDescendantOfTarget = targetPath.empty();

		// enter down to target
		while (!targetPath.empty())
		{
			mCurrentState = targetPath.back();
			targetPath.pop_back();
			if (mOnEntry)
			{
				mOnEntry(*this);
			}
			if (mCurrentState->entry)
			{
				mCurrentState->entry(*this);
			}
		}

		if (!wasDescendantOfTarget && mCurrentState->initialTransition.target)
		{
			// perform initial transition
			return DoTransition(mCurrentState->initialTransition);
		}
		else
		{
			return true;
		}
	}

	template<typename OwnerType, typename EventType>
	bool StateMachine<OwnerType, EventType>::IsInState(const State& s) const
	{
		// check if 's' is in our current state's lineage
		const State* cs = mCurrentState;
		while (cs)
		{
			if (cs == &s)
			{
				return true;
			}
			cs = cs->parent;
		}

		return false;
	}

	template<typename OwnerType, typename EventType>
	void StateMachine<OwnerType, EventType>::LogEntry(Severity severity, const char* format, ...)
	{
		const std::string severityLabels[] = { "","WARNING| ", "ERROR| " };
		std::string decoratedFormat = severityLabels[severity] + format;
		va_list args;
		va_start(args, format);
		mLog(decoratedFormat.c_str(), args);
		va_end(args);
	}

} // namespace Hsm2
