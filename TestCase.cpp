// Copyright 2016, Jason Conaway
#include "TestCase.h"

#include <cstdarg>
#include <iostream>
#include <string>

///////////////////////////////////////////////////////////////////////////////
// TestCase state definitions

// Testing
const TestCase::State TestCase::Testing
{
	Name("Testing")
	.Initially(StartIn(Alpha))
	.Always(When("Reset").Goto(Alpha).Do(ResetCounter))
};

// Testing.Alpha
const TestCase::State TestCase::Alpha
{
	Name("Alpha").Parent(Testing)
	.OnEntry(IncCounter)
	.Always(When("Jump").Goto(Beta))
	.Always(When("Run").Goto(Beta).Do(IncCounter))
};

// Testing.Beta
const TestCase::State TestCase::Beta
{
	Name("Beta").Parent(Testing)
	.OnEntry(IncCounter)
	.Initially(StartIn(Gamma).Do(IncCounter))
	.Always(When("Hide").Do(IncCounter))
};

// Testing.Beta.Gamma
const TestCase::State TestCase::Gamma
{
	Name("Gamma").Parent(Beta)
	.OnExit(IncCounter)
};

///////////////////////////////////////////////////////////////////////////////
// TestCase state machine actions

/*static*/ void TestCase::OnEntry(Hsm& hsm)
{
	std::cout << "TestCase| entered state " << hsm.CurrentState().name << std::endl;
}

/*static*/ void TestCase::OnExit(Hsm& hsm)
{
	std::cout << "TestCase| exited state " << hsm.CurrentState().name << std::endl;
}

/*static*/ void TestCase::IncCounter(Hsm& hsm)
{
	std::cout << "TestCase| counter = " << ++hsm.GetOwner().counter << std::endl;
}

/*static*/ void TestCase::ResetCounter(Hsm& hsm)
{
	hsm.GetOwner().counter = 0;
	std::cout << "TestCase| counter reset to = " << hsm.GetOwner().counter << std::endl;
}

///////////////////////////////////////////////////////////////////////////////
// TestCase methods

TestCase::TestCase()
{
	mStateMachine.Initialize();
	mStateMachine.OnEntryAndExit(OnEntry, OnExit);
}

/*static*/ void TestCase::Log(const char* format, va_list args)
{
	fprintf(stderr, "TestCase| ");
	vfprintf(stderr, format, args);
	fprintf(stderr, "\n");
}
