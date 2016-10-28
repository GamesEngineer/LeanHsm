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
	Name("Closed").Parent(Exists)
	.Initially(StartIn(Unlocked))
};

// Exists.Closed.Unlocked
const Door::State Door::Unlocked
{
	Name("Unlocked").Parent(Closed)
	.Always(When(Event::Lock).Goto(Locked).Do(PlayFx("LockingDoor")))
	.Always(When(Event::Open).Goto(Opened).Do(PlayFx("OpeningDoor")))
};

// Exists.Closed.Locked
const Door::State Door::Locked
{
	Name("Locked").Parent(Closed)
	.OnEntry(LockedLightOn)
	.OnExit(LockedLightOff)
	.Always(When(Event::Unlock).Goto(Unlocked).Do(PlayFx("UnlockingDoor")))
	.Always(When(Event::Open).Do(PlayFx("RattleLockedDoor")))
};

// Exists.Opened
const Door::State Door::Opened
{
	Name("Opened").Parent(Exists)
	.Always(When(Event::Close).Goto(Closed).Do(PlayFx("ClosingDoor")))
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

/*static*/ void Door::LockedLightOn(Hsm& hsm)
{
	std::cout << "Door| light on" << std::endl;
}

/*static*/ void Door::LockedLightOff(Hsm& hsm)
{
	std::cout << "Door| light off" << std::endl;
}

/*static*/ Door::Hsm::Action Door::PlayFx(const std::string& effectName)
{
	auto action = [effectName](Hsm& hsm) {
		std::cout << "Door| playing effect '" << effectName << "'" << std::endl;
		hsm.Owner<Door>().mCurrentEffect = effectName;
		return;
	};

	return action;
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