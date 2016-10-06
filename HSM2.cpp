// HSM2.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include <iostream>
#include <string>
#include "StateMachine.h"

class TestCase;

using Event = std::string;
using Hsm = Hsm2::StateMachine<TestCase, Event>;
using State = Hsm::State;
using Name = Hsm::Name;
using StartIn = Hsm::StartIn;
using When = Hsm::When;
//using If = Hsm::If;

///////////////////////////////////////////////////////////////////////////////

class TestCase
{
public:
	bool Test()
	{
		sm.Initialize();
		bool t = sm.HandeleEvent("Jump");
		t = t && sm.HandeleEvent("Reset");
		return t;
	}
private:
	static const State Testing;
	static const State /**/Alpha;
	static const State /**/Beta;
	static const State /****/Gamma;

	static void IncCounter(Hsm& hsm);
	static bool CounterIsNegative(Hsm& hsm);

	Hsm sm{ *this, Testing };
	int counter{ 0 };
};

/*static*/ void TestCase::IncCounter(Hsm& hsm) { std::cout << ++hsm.GetOwner().counter << std::endl; }
/*static*/ bool TestCase::CounterIsNegative(Hsm& hsm) { return hsm.GetOwner().counter > 0; }

///////////////////////////////////////////////////////////////////////////////

const State TestCase::Testing
{
	Name("Testing")
	.Initially(StartIn(Alpha))
};

// Testing.Alpha
const State TestCase::Alpha
{
	Name("Alpha")
	.Entry(IncCounter)
	.Always(When("Jump").Goto(Beta))
	.Always(When("Run").Goto(Beta).Do(IncCounter))
};

// Testing.Beta
const State TestCase::Beta
{
	Name("Beta")
	.Entry(IncCounter)
	.Initially(StartIn(Gamma).Do(IncCounter))
	.Always(When("Hide").Goto(Alpha))
	//.Conditionally(If(CounterIsNegative)
	//	.Then(When("Check").Goto(Alpha).Do(IncCounter))
	//	.Else(When("Check").Goto(Alpha))
	//	)
};

// Testing.Beta.Gamma
const State TestCase::Gamma
{
	Name("Gamma")
	.Parent(Beta)
	.Exit(IncCounter)
	.Always(When("Reset").Goto(Alpha).Do(IncCounter))
};

///////////////////////////////////////////////////////////////////////////////

int main()
{
	TestCase t;
	std::cout << "Test result: " << t.Test() << std::endl;
	return 0;
}

