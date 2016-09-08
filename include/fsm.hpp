#pragma once

#include "logger.hpp"
#include "types.hpp"
#include <functional>
#include <memory>
#include <vector>
#include <set>
#include <map>

// Maps strings to functions
template<class ReturnType, class FunctionType>
class FunctionMap final
{
public:
    using TEventFuncPtr = std::function < ReturnType(FunctionType) >;

    void Add(const char* functionName, TEventFuncPtr function)
    {
        const auto it = mFunctions.find(method);
        if (it != std::end(mFunctions))
        {
            LOG_ERROR("Function: " << method.c_str() << " already added");
        }
        else
        {
            mFunctions.insert(std::make_pair(functionName, function));
        }
    }

    ReturnType Evaluate(const std::string& method, class FsmArgumentStack& stack)
    {
        const auto it = mFunctions.find(method);
        if (it != std::end(mFunctions))
        {
            return it->second(stack);
        }
        LOG_ERROR("Missing function: " << method.c_str());
        return ReturnType();
    }

private:
    std::map<std::string, TEventFuncPtr> mFunctions;
};

// This is only sort of a stack, you push arguments for the state action/condition, then during
// FSM execution the pops happen but don't actually remove anything, and before any pops reset is called
// which makes up start "fake" popping from the top of the stack again.
class FsmArgumentStack final
{
public:
    FsmArgumentStack() = default;
    void Reset();
    void Push(std::string str);
    void Push(s32 integer);
    const std::string& PopString();
    s32 PopInt();
private:
    s32 mStringPos = 0;
    std::vector<std::string> mStringArgs;
    s32 mIntPos = 0;
    std::vector<s32> mIntArgs;
};

// Calls a function from FunctionMap by name, and allows adding
// of arguments to be passed to said function via a stack.
template<class T, class ReturnType>
class FsmExecutor final
{
public:
    FsmExecutor(const std::string& name) : mName(name) { }

    ReturnType Execute(T& functions)
    {
        mArguments.Reset();
        return functions.Evaluate(mName, mArguments);
    }

    FsmArgumentStack& Arguments() { return mArguments; }
private:
    std::string mName;
    FsmArgumentStack mArguments;
};

using TActions = FunctionMap < void, FsmArgumentStack& >;
using TConditions = FunctionMap < bool, FsmArgumentStack& >;
using StateCondition = FsmExecutor < TConditions, bool >;
using StateAction = FsmExecutor < TActions, void >;

class StateTransition final
{
public:
    StateTransition(const std::string& name) : mTargetStateName(name) { }
    void AddCondition(StateCondition condition);
    bool Evaulate(TConditions& events);
    const std::string& Name() const { return mTargetStateName; }
private:
    std::string mTargetStateName;
    std::vector<StateCondition> mConditions;
};

class State final
{
public:
    State(const std::string& name) : mStateName(name) { }
    void Enter(TActions& actions);
    const std::string* Update(TConditions& states);
    const std::string& Name() const { return mStateName; }
    void AddEnterAction(StateAction action) { mEnterActions.push_back(action); }
    void AddTransition(StateTransition transition) { mTransistions.push_back(transition); }
private:
    std::string mStateName;
    std::vector<StateAction> mEnterActions;
    std::vector<StateTransition> mTransistions;
};

class StateMachine final
{
public:
    StateMachine() = default;
    void Construct();
    void Update();
    bool ToState(const char* stateName);
    TConditions& Conditions() { return mConditions; }
    TActions& Actions() { return mActions; }
private:
    std::vector<State> mStates;
    TConditions mConditions;
    TActions mActions;
    State* mActiveState = nullptr;
};
