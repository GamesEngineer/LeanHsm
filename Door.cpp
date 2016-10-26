// Copyright 2016, Jason Conaway
#include "Door.h"

#include <cstdarg>
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// Door state definitions

// Exists
const Door::State Door::Exists
{
	Name("Exists")
	.Initially(StartIn(Closed))
};

// Exists.Closed
const Door::State Door::Closed
{
	Name("Closed")
	.Parent(Exists)
	.Initially(StartIn(Unlocked))
};

// Exists.Closed.Locked
const Door::State Door::Locked
{
	Name("Locked")
	.Parent(Closed)
	.Always(When(Event::Unlock).Goto(Unlocked))
};

// Exists.Closed.Unlocked
const Door::State Door::Unlocked
{
	Name("Unlocked")
	.Parent(Closed)
	.Always(When(Event::Lock).Goto(Locked))
	.Always(When(Event::Open).Goto(Opened))
};

// Exists.Opened
const Door::State Door::Opened
{
	Name("Opened")
	.Parent(Exists)
	.Always(When(Event::Close).Goto(Closed))
};

///////////////////////////////////////////////////////////////////////////////
// Door state machine actions

/*static*/ void Door::OnEntry(Hsm& hsm)
{
	std::cout << "Door| entered state " << hsm.CurrentState().name << std::endl;
}

/*static*/ void Door::OnExit(Hsm& hsm)
{
	std::cout << "Door| exited state " << hsm.CurrentState().name << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// Door methods

Door::Door()
{
	mStateMachine.Initialize();
	mStateMachine.OnEntryAndExit(OnEntry, OnExit);
}

/*static*/ std::string Door::EventToString(Event e)
{
	const char* eventNames[]{ "Open", "Close", "Lock", "Unlock" };
	return eventNames[int(e)];
}

/*static*/void Door::Log(const char* format, va_list args)
{
	fprintf(stderr, "Door| ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
}