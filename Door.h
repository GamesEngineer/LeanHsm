// Copyright 2016, Jason Conaway
#pragma once

#include "StateMachine.h"

class Door
{
public:
	enum class Event : int
	{
		Open,
		Close,
		Lock,
		Unlock
	};

	LEAN_HSM_ALIASES(Door, Event);

	Door();

	bool HandleEvent(Event e) { return mStateMachine.HandeleEvent(e); }
	bool IsInState(const State& s) const { return mStateMachine.IsInState(s); }
	const State& GetState() const { return mStateMachine.CurrentState(); }

	// States
	static const State Exists;
	static const State /**/Closed;
	static const State /****/Locked;
	static const State /****/Unlocked;
	static const State /**/Opened;

	static void Log(const char* format, va_list args);
	static std::string EventToString(Event e);

private:
	// Actions
	static void OnEntry(Hsm& hsm);
	static void OnExit(Hsm& hsm);

	Hsm mStateMachine{ *this, Exists, Log, EventToString };
};
