// Copyright 2016, Jason Conaway
// LeanHsmTest.cpp
// This is the entry point for a console application that demonstates
// a flexible and efficient hierarchical finite state machine class.
//

#include <iostream>
#include <limits>
#include <string>

#include "Door.h"
#include "TestCase.h"

///////////////////////////////////////////////////////////////////////////////

bool Test_Door();
bool Test_TestCase();

int main()
{
	std::cout
		<< "Door| Test result: "
		<< (Test_Door() ? "SUCCESS" : "FAILURE")
		<< std::endl << std::endl;

	std::cout
		<< "TestCase| Test result: "
		<< (Test_TestCase() ? "SUCCESS" : "FAILURE")
		<< std::endl << std::endl;

	std::cout << "Press ENTER to continue... " << std::flush;
	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	return 0;
}

///////////////////////////////////////////////////////////////////////////////

#define REQUIRE_TRUE(x) { if (!(x)) return false; }
#define REQUIRE_FALSE(x) { if (x) return false; }

bool Test_Door()
{
	using Event = Door::Event;

	Door door;
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Unlocked));

	REQUIRE_TRUE(door.HandleEvent(Event::Lock));
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Locked));
	REQUIRE_TRUE(door.GetCurrentEffect() == "LockingDoor");

	// Must not be openable when locked, but will play the rattle effect
	REQUIRE_TRUE(door.HandleEvent(Event::Open));
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Locked));
	REQUIRE_TRUE(door.GetCurrentEffect() == "RattleLockedDoor");

	REQUIRE_TRUE(door.HandleEvent(Event::Unlock));
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Unlocked));
	REQUIRE_TRUE(door.GetCurrentEffect() == "UnlockingDoor");

	REQUIRE_TRUE(door.HandleEvent(Event::Open));
	REQUIRE_TRUE(door.IsInState(door.Opened));
	REQUIRE_TRUE(door.GetCurrentEffect() == "OpeningDoor");

	// Must not be lockable when opened
	REQUIRE_FALSE(door.HandleEvent(Event::Lock));

	REQUIRE_TRUE(door.HandleEvent(Event::Close));
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Unlocked));
	REQUIRE_TRUE(door.GetCurrentEffect() == "ClosingDoor");

	REQUIRE_TRUE(door.HandleEvent(Event::Lock));
	REQUIRE_TRUE(door.IsInState(door.Closed));
	REQUIRE_TRUE(door.IsInState(door.Locked));
	REQUIRE_TRUE(door.GetCurrentEffect() == "LockingDoor");

	return true; // passed all requirements
}

bool Test_TestCase()
{
	TestCase testCase;
	REQUIRE_TRUE(testCase.HandleEvent("Jump"));
	REQUIRE_TRUE(testCase.HandleEvent("Hide"));
	REQUIRE_TRUE(testCase.HandleEvent("Reset"));

	return true; // passed all requirements
}