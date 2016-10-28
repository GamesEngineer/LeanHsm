// Copyright 2016, Jason Conaway
// This is a simple model of a door using the LeanHsm::StateMachine.
// The door can be closed or open, and when closed it can be locked or unlocked.
#pragma once

#include "StateMachine.h"

class Door
{
public:
	enum class Event
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
	const std::string& GetCurrentEffect() const { return mCurrentEffect; }

	// States
	static const State Exists;
	static const State /**/Closed;
	static const State /****/Locked;
	static const State /****/Unlocked;
	static const State /**/Opened;

private:
	// Methods required by the state machine 
	static void Log(const char* format, va_list args);
	static std::string EventToString(Event e);

	// Actions
	static void OnEntry(Hsm& hsm);
	static void OnExit(Hsm& hsm);
	static void LockedLightOn(Hsm& hsm);
	static void LockedLightOff(Hsm& hsm);
	static Hsm::Action PlayFx(const std::string& effectName);

	OwnedHsm mStateMachine{ *this, Exists, Log, EventToString };
	std::string mCurrentEffect;
};
