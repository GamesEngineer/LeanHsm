// Copyright 2016, Jason Conaway
#pragma once

#include <algorithm>
#include <functional>
#include <iterator>
#include <vector>

namespace Hsm2
{
	template<typename OwnerType, typename EventType>
	class StateMachine
	{
	public:
		struct State;
		struct Transition;
		struct StartIn;
		struct When;
		//struct If;

		using Event = EventType;
		using Action = std::function<void(StateMachine& owner)>;
		using Transitions = std::vector<Transition>;
		using Guard = std::function<bool(StateMachine& owner)>;
		//using Conditionals = std::vector<If>;

		struct State
		{
			State Parent(const State& s) && { parent = &s; return std::move(*this);	}
			State Entry(const Action& a) && { entry = a; return std::move(*this); }
			State Exit(const Action& a) && { exit = a; return std::move(*this); }
			State Initially(StartIn&& t) && { initialTransition = std::move(t);	return std::move(*this); }
			State Always(When&& t) && { transitions.emplace_back(std::forward<Transition>(t)); return std::move(*this); }
			
			// Use conditional transitions very sparingly
			//State Conditionally(If&& c) && { conditionals.emplace_back(std::forward<If>(c)); return std::move(*this); }

			const char* name{ nullptr };
			const State* parent{ nullptr };
			Action entry{ nullptr };
			Action exit{ nullptr };
			StartIn initialTransition;
			Transitions transitions;
			//Conditionals conditionals;

			State(State&&) = default;
			State& operator=(State&&) = default;
		protected:
			explicit State(const char* n) : name(n) {} // For the Name type
		};

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

		/***
		struct If
		{
			explicit If(const Guard& c) : guard(c) {}
			If Then(Transition&& t) { onTrue = std::move(t); return std::move(*this); }
			If Else(Transition&& t) { onFalse = std::move(t); return std::move(*this); }
			Gaurd guard;
			Transition onTrue;
			Transition onFalse;
		};
		***/

		///

		StateMachine(OwnerType& o, const State& s) : mOwner(o), mCurrentState(&s) {}
		void Initialize() { if (mCurrentState) { DoTransition(mCurrentState->initialTransition); } }
		OwnerType& GetOwner() const { return mOwner; }
		const State& CurrentState() const { return *mCurrentState; }
		bool HandeleEvent(const EventType& e);
		
	private:
		std::vector<const State*> GetCommonAncestorPath(const State* a, const State* b) const;
		void DoTransition(const Transition& t);

		OwnerType& mOwner;
		const State* mCurrentState{ nullptr };
	};

	template<typename OwnerType, typename EventType>
	std::vector<typename const StateMachine<OwnerType, EventType>::State*> StateMachine<OwnerType, EventType>::GetCommonAncestorPath(const State* a, const State* b) const
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
		if (!mCurrentState) { return false; }
		auto state = mCurrentState;

		// find transition
		auto transition = std::find_if(
			begin(state->transitions)
			end(state->transitions),
			[e](const Transition& t) { return t.eventId == e; });
		if (transition != end(state->transitions))
		{
			DoTransition(*transition);
			return true;
		}
		// TODO - support conditionals

		return false;
	}

	template<typename OwnerType, typename EventType>
	void StateMachine<OwnerType, EventType>::DoTransition(const Transition& transition)
	{
		if (!mCurrentState || !transition.target) { return; }

		// exit up to common ancestor
		auto targetPath = GetCommonAncestorPath(mCurrentState, transition.target);
		if (!targetPath.empty())
		{
			auto ancestor = targetPath.back();
			targetPath.pop_back();
			while (mCurrentState != ancestor)
			{
				if (mCurrentState->exit)
				{
					mCurrentState->exit(*this);
				}
				mCurrentState = mCurrentState->parent;
			}
		}

		// do transition action
		if (transition.action)
		{
			transition.action(*this);
		}

		// enter down to target
		while (!targetPath.empty())
		{
			mCurrentState = targetPath.back();
			targetPath.pop_back();
			if (mCurrentState->entry)
			{
				mCurrentState->entry(*this);
			}
		} 
	}

} // namespace Hsm2
