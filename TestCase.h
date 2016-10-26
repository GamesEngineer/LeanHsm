// Copyright 2016, Jason Conaway
#pragma once

#include "StateMachine.h"

class TestCase
{
public:

	using Event = std::string;
	LEAN_HSM_ALIASES(TestCase, Event);

	TestCase();

	bool HandleEvent(const Event& e) { return mStateMachine.HandeleEvent(e); }
	bool IsInState(const State& s) const { return mStateMachine.IsInState(s); }
	const State& GetState() const { return mStateMachine.CurrentState(); }

	static void Log(const char* format, va_list args);
	static std::string EventToString(Event e) { return e; }

private:
	static const State Testing;
	static const State /**/Alpha;
	static const State /**/Beta;
	static const State /****/Gamma;

	static void OnEntry(Hsm& hsm);
	static void OnExit(Hsm& hsm);
	static void IncCounter(Hsm& hsm);
	static void ResetCounter(Hsm& hsm);

	Hsm mStateMachine{ *this, Testing, Log, EventToString };
	int counter{ 0 };
};
