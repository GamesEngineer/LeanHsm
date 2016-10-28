// Copyright 2016, Jason Conaway
// LeanHsm - The flexible and efficient hierachical state machine
//
// USAGE:
// Classes that have an OwnedStateMachine should use the LEAN_HSM_ALIASES macro
// to define aliases in the public scope of the class. This helps to keep the
// state definitions compact and readable.
//
// The owning class should also declare its states as static members. The
// definitions of those states should use the following pattern:
//
// const Owner::State Owner::SomeState {
//     /** Name of the state, used for logging **/
//     Name("SomeState")
//
//     /** Parent state (must be omitted for top-level state) **/
//     .Parent(MyParentState)
//
//     /** (optional) Action to be invoked when entering this state. **/
//     .OnEntry(SomeStaticMethodOfOwner)
//
//      /** (optional) Action to be invoked when leaving this state. **/
//     .OnExit(AnotherStaticMethodOfOwner)
//
//     /** Initial transition to a sub-state, occurs when entering this state. **/
//     .Initially(StartIn(MySubState))
//
//     /** Transitions to other states. Actions are optional. **/
//     .Always(When(Event::Poke).Goto(YetAnotherState).Do(MoreStaticMethodOfOwner))
//     .Always(When(Event::Prod).Goto(YetAnotherState)
//
//     /** Internal state transitions; stay in the same state, and do an action. **/
//     .Always(When(Event::Pull).Do(MoreStaticMethodOfOwner))
// };
//
// When HandleEvent(event) is called on the state machine, the correct transition
// will be performed from the current state to the transition's target state.
//
// Order of operations when performing a state transition:
// 1) Exit states, starting with the current state, up to the common ancestor of
//    the current state and the target state. The OnExit action of each of exited
//    state is invoked in sequence.
// 2) The transition's action (if any) is invoked while in the least common
//    ancestor (LCA) state.
// 3) Enter states down to the target state. The OnEnter action of each entered
//    state is invoked, ending with (and including) the target state.
// 4) If the state that owns this transition was not a descendant of the target
//    state, then the initial transition of the target state is invoked.
//
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>
#include <cstdarg>

// Owners of state machines should use this macro to define
// aliases in their public scope. Then, they should have an
// instance of OwnedHsm as their state machine, which is
// initialized with the top state of a state graph.
#define LEAN_HSM_ALIASES(OwnerType, EventType) \
	using OwnedHsm = LeanHsm::OwnedStateMachine<OwnerType, EventType>; \
	using Hsm = LeanHsm::StateMachine<EventType>; \
	using State = Hsm::State; \
	using Name = Hsm::Name; \
	using StartIn = Hsm::StartIn; \
	using When = Hsm::When;

namespace LeanHsm
{

template<typename EventType>
class StateMachine
{
public:
	struct State;
	struct Transition;
	struct StartIn;
	struct When;

	using Event = EventType;
	using Action = std::function<void(StateMachine& sm)>;
	using Transitions = std::vector<Transition>;
	using Guard = std::function<bool(StateMachine& sm)>;
	using EventToString = std::function<std::string (EventType e)>;
	using Log = std::function<void(const char* format, va_list args)>;

	// States reference each other via Transitions and Parents to form
	// a hierachical state graph. The StateMachine handles events to
	// tigger transitions among states in the graph, performing actions
	// along the way.
	struct State
	{
		// rvalue methods for initializing state properties
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

	// Transition is a generic state transition.
	// Use the StartIn and When derivations when defining states.
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

	// StartIn is a used with State::Initially to create an intial transition
	struct StartIn : public Transition
	{
		StartIn() = default; // for states that omit initial transitions
		explicit StartIn(const State& s) : Transition(s) {}
		// Use Do to specify transition actions. (Optional) 
		StartIn Do(const Action& a) && { action = a; return std::move(*this); }
	};

	// When is used with State::Always to create a normal state transition
	struct When : public Transition
	{
		explicit When(const EventType& e) : Transition(e) {}
		// Use Goto for normal transitions. Omit it for internal transitions.
		When Goto(const State& s) && { target = &s; return std::move(*this); }
		// Use Do to specify transition actions. (Optional) 
		When Do(const Action& a) && { action = a; return std::move(*this); }
	};

	///////////////////////////////////////////////////////////////////////
	// StateMachine methods

	StateMachine(const State& topState, const Log& log, const EventToString& e2s)
		: mCurrentState(&topState), mLog(log), mEventToString(e2s) {}
		
	// Specifies additional entry and exit actions for all states.
	// These are invoked before state entry/exit actions.
	void OnEntryAndExit(Action entry, Action exit) { mOnEntry = entry; mOnExit = exit; }
		
	// Initializes the state machine by transitioning to the initial state.
	void Initialize() { if (mCurrentState) { DoTransition(mCurrentState->initialTransition); } }
		
	// Returns the current state
	const State& CurrentState() const { return *mCurrentState; }
		
	// Returns true if the current state is in the specified state.
	// This includes ancestor states. Returns false, otherwise.
	bool IsInState(const State& s) const;

	// Finds a state transition in the current state that is associated with
	// this event, and performs the state transition. If matching transition
	// was found, then this funtion returns true. Returns false, otherwise.
	bool HandeleEvent(const EventType& e);

	// Returns the owner object when this is an OwnedStateMachine.
	// This is used by Actions that need a reference to their owner.
	template<typename OwnerType> OwnerType& Owner() const;

private:
	std::vector<const State*> GetCommonAncestorPath(const State* a, const State* b) const;
	bool DoTransition(const Transition& t);
	enum Severity { Info, Warning, Error };
	void LogEntry(Severity severity, const char* format, ...);

	const State* mCurrentState{ nullptr };
	Action mOnEntry;
	Action mOnExit;
	Log mLog;
	EventToString mEventToString;
};

// OwnedStateMachine is a StateMachine with an owner object.
// Actions often need to access to their owner, and this provides access.
template<typename OwnerType, typename EventType>
class OwnedStateMachine : public StateMachine<EventType>
{
public:
	OwnedStateMachine(OwnerType& owner, const State& topState, const Log& log, const EventToString& e2s)
		: StateMachine(topState, log, e2s), mOwner(owner) {}
	OwnerType& GetOwner() const { return mOwner; }
private:
	OwnerType& mOwner;
};

///////////////////////////////////////////////////////////////////////////
// StateMachine implementation

template<typename EventType>
template<typename OwnerType>
OwnerType& StateMachine<EventType>::Owner() const
{
	using OHSM = OwnedStateMachine<OwnerType, EventType>;
	return static_cast<const OHSM&>(*this).GetOwner();
}

template<typename EventType>
std::vector<typename const StateMachine<EventType>::State*>
	StateMachine<EventType>::GetCommonAncestorPath(const State* a, const State* b) const
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

template<typename EventType>
bool StateMachine<EventType>::HandeleEvent(const EventType& e)
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
			LogEntry(Info, "event [%s]", mEventToString(e).c_str());
			return DoTransition(*transition);
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

template<typename EventType>
bool StateMachine<EventType>::DoTransition(const Transition& transition)
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
	LogEntry(Info, "transition %s -> %s", mCurrentState->name, target->name);

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

template<typename EventType>
bool StateMachine<EventType>::IsInState(const State& s) const
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

template<typename EventType>
void StateMachine<EventType>::LogEntry(Severity severity, const char* format, ...)
{
	const std::string severityLabels[] = { "","WARNING| ", "ERROR| " };
	std::string decoratedFormat = severityLabels[severity] + format;
	va_list args;
	va_start(args, format);
	mLog(decoratedFormat.c_str(), args);
	va_end(args);
}

} // namespace LeanHsm
