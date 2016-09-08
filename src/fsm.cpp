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

FiniteStateMachine::FiniteStateMachine(sol::state& luaState)
    : mLuaState(luaState)
{

}

void FiniteStateMachine::Construct()
{
    const char* kScript = R"(
function SetAnimation(animation)
    print("Animation is " .. animation)
end

function InputRight()
    return true
end

function FacingLeft()
    return true
end

function IsAnimationComplete()
    return true
end

function PlaySoundEffect(sound)
    print("Sound is " .. sound)
end

local states = {}

function states.Idle()
   SetAnimation("AbeStandIdle")
   if InputRight() then
      states.active = states.ToWalk
   end
end

function states.ToWalk()
   SetAnimation("AbeStandToWalk")
   if IsAnimationComplete() then
     states.active = states.Walking
   end
end

function states.Walking()
   SetAnimation("AbeWalking")
   PlaySoundEffect("Walking")
   if not InputRight() then
      states.active = states.ToIdle
   end
end

function states.ToIdle()
   SetAnimation("AbeWalkToStand")
   if IsAnimationComplete() then
     states.active = states.Idle
   end
end

states.active = states.Idle;
states.active()

local machine = require('statemachine')

local fsm = machine.create({
  initial = 'Stand',
  events = {
    { name = 'Stand', to = 'ToWalking' animation = 'AbeStandIdle'
         condition = function() 
            if InputRight() and not FacingLeft() then return true else return false end
        end
    },
    { name = 'Stand',      to = 'Turn'       },
    { name = 'ToWalking',  to = 'Walking'    },
    { name = 'Walking',    to = 'ToStand'    },
    { name = 'ToStand',    to = 'Stand'      },

}})

)";
    mLuaState.script(kScript);

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
