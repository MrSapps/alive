#include "fsm.hpp"

// =========================================================================

void FsmArgumentStack::Reset()
{
    mStringPos = 0;
    mIntPos = 0;
}

void FsmArgumentStack::Push(std::string str)
{
    mStringArgs.push_back(str);
}

void FsmArgumentStack::Push(s32 integer)
{
    mIntArgs.push_back(integer);
}

const std::string& FsmArgumentStack::PopString()
{
    return mStringArgs[mStringPos++];
}

s32 FsmArgumentStack::PopInt()
{
    return mIntArgs[mIntPos++];
}

// =========================================================================

void StateTransition::AddCondition(StateCondition condition)
{
    mConditions.push_back(condition);
}

bool StateTransition::Evaulate(TConditions& events)
{
    // Nothing to check, just move to next state
    if (mConditions.empty())
    {
        return true;
    }

    for (StateCondition& event : mConditions)
    {
        if (!event.Execute(events))
        {
            // An event isn't true so can't go to next state yet
            return false;
        }
    }

    // All events are true, go to next state
    return true;
}

// =========================================================================

void State::Enter(TActions& actions)
{
    LOG_INFO("Enter state:" << mStateName.c_str());
    for (StateAction& action : mEnterActions)
    {
        action.Execute(actions);
    }
}

const std::string* State::Update(TConditions& states)
{
    // TODO: Need "Running" actions - for playing sound effects
    // per anim frame?

    for (StateTransition& trans : mTransistions)
    {
        if (trans.Evaulate(states))
        {
            return &trans.Name();
        }
    }
    return nullptr;
}

// =========================================================================

void StateMachine::Update()
{
    if (mActiveState)
    {
        const std::string* nextState = mActiveState->Update(mConditions);
        if (nextState)
        {
            ToState(nextState->c_str());
        }
    }
}

bool StateMachine::ToState(const char* stateName)
{
    for (State& state : mStates)
    {
        if (state.Name() == stateName)
        {
            mActiveState = &state;
            mActiveState->Enter(mActions);
            return true;
        }
    }

    LOG_ERROR("State: " << stateName << " not found");
    return false;
}

void StateMachine::Construct()
{
    // All of this will come from a config file

    {
        State state("Idle");

        StateAction action("SetAnimation");
        action.Arguments().Push("TheIdleAnim");

        state.AddEnterAction(action);

        StateCondition animComplete = "IsAnimationComplete";

        StateTransition trans("ToWalk");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        State state("ToWalk");

        StateAction action("SetAnimation");
        action.Arguments().Push("TheToWalkAnim");

        state.AddEnterAction(action);

        StateCondition animComplete = "IsAnimationComplete";

        StateTransition trans("Walking");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        State state("Walking");

        StateAction action("SetAnimation");
        action.Arguments().Push("TheWalkingAnm");

        StateAction sound("PlaySoundEffect");
        sound.Arguments().Push("Walk effect");

        state.AddEnterAction(action);
        state.AddEnterAction(sound);

        StateCondition animComplete("IsAnimationComplete");

        StateTransition trans("ToIdle");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        State state("ToIdle");

        StateAction action("SetAnimation");
        action.Arguments().Push("TheToIdleAnim");

        state.AddEnterAction(action);

        StateCondition animComplete("IsAnimationComplete");

        StateTransition trans("Idle");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    ToState("Idle");
}
