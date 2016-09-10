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

void FsmStateTransition::AddCondition(StateCondition condition)
{
    mConditions.push_back(condition);
}

bool FsmStateTransition::Evaulate(TConditions& events)
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

void FsmState::Enter(TActions& actions)
{
    LOG_INFO(mStateName.c_str());
    for (StateAction& action : mEnterActions)
    {
        action.Execute(actions);
    }
}

const std::string* FsmState::Update(TConditions& states)
{
    // TODO: Need "Running" actions - for playing sound effects
    // per anim frame?

    for (FsmStateTransition& trans : mTransistions)
    {
        if (trans.Evaulate(states))
        {
            return &trans.Name();
        }
    }
    return nullptr;
}

// =========================================================================

void FiniteStateMachine::Update()
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

bool FiniteStateMachine::ToState(const char* stateName)
{
    for (FsmState& state : mStates)
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

void FiniteStateMachine::Construct()
{

    // All of this will come from a config file

    {
        FsmState state("Idle");

        StateAction action("SetAnimation");
        action.Arguments().Push("AbeStandIdle");


        state.AddEnterAction(action);

       
        FsmStateTransition trans("ToWalk");
       
        StateCondition pressRight("InputRight");
        trans.AddCondition(pressRight);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        FsmState state("ToWalk");

        StateAction action("SetAnimation");
        action.Arguments().Push("AbeStandToWalk");

        state.AddEnterAction(action);

        StateCondition animComplete("IsAnimationComplete");

        FsmStateTransition trans("Walking");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        FsmState state("Walking");

        StateAction action("SetAnimation");
        action.Arguments().Push("AbeWalking");

        StateAction sound("PlaySoundEffect");
        sound.Arguments().Push("Walk effect");

        state.AddEnterAction(action);
        state.AddEnterAction(sound);

        FsmStateTransition trans("ToIdle");
        StateCondition pressRight("!InputRight");
        trans.AddCondition(pressRight);


        state.AddTransition(trans);

        mStates.push_back(state);
    }

    {
        FsmState state("ToIdle");

        StateAction action("SetAnimation");
        action.Arguments().Push("AbeWalkToStand");

        state.AddEnterAction(action);

        StateCondition animComplete("IsAnimationComplete");

        FsmStateTransition trans("Idle");
        trans.AddCondition(animComplete);

        state.AddTransition(trans);

        mStates.push_back(state);
    }

    ToState("Idle");
}
