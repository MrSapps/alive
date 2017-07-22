#pragma once

#include "types.hpp"
#include <functional>
#include <chrono>

constexpr u32 kMaxExecutionTimeMs = 8;

class Timer
{
public:
    Timer() : mStartTime(TClock::now())
    {

    }

    void Reset()
    { 
        mStartTime = TClock::now();
    }

    u32 Elapsed() const 
    {
        const u32 ret = static_cast<u32>(std::chrono::duration_cast<std::chrono::milliseconds>(TClock::now() - mStartTime).count());
        return ret;
    }

private:
    using TClock = std::chrono::high_resolution_clock;
    std::chrono::time_point<TClock> mStartTime;
};

template<class Func>
inline void RunForAtLeast(u32 millisecondsTimeBox, Func func)
{
    Timer time;
    do
    {
        func();
    } while (time.Elapsed() < millisecondsTimeBox);
}

template<class LoopIndexType>
class IterativeForLoop
{
public:
    IterativeForLoop() = default;
    ~IterativeForLoop() = default;

    LoopIndexType Value() const
    {
        return mCurrentIndex;
    }

    template<class Func>
    bool IterateTimeBoxed(u32 millisecondsTimeBox, LoopIndexType bound, Func loopIterationFunc)
    {
        Timer time;
        bool iterationCompleted = false;
        do
        {
            iterationCompleted = IterateCommon(bound, [&]()
            {
                // For the most nested iterate we run the loop body and always increment
                loopIterationFunc();
                return true;
            });
            if (iterationCompleted)
            {
                break;
            }
        } while (time.Elapsed() < millisecondsTimeBox);
        return iterationCompleted;
    }

    template<class Func>
    bool Iterate(LoopIndexType bound, Func loopIterationFunc)
    {
        return IterateCommon(bound, [&]()
        {
            // For the most nested iterate we run the loop body and always increment
            loopIterationFunc();
            return true;
        });
    }

    template<class Func>
    bool IterateIf(LoopIndexType bound, Func loopIterationFunc)
    {
        return IterateCommon(bound, [&]()
        {
            // The caller supplied loop function determines if we increment, typically
            // this is the result of another Iterate, i.e the outer loop only increments
            // if the inner loop iteration is done.
            return loopIterationFunc();
        });
    }

    template<class Func>
    bool IterateTimeBoxedIf(u32 millisecondsTimeBox, LoopIndexType bound, Func loopIterationFunc)
    {
        Timer time;
        bool iterationCompleted = false;
        do
        {
            iterationCompleted = IterateCommon(bound, [&]()
            {
                // The caller supplied loop function determines if we increment, typically
                // this is the result of another Iterate, i.e the outer loop only increments
                // if the inner loop iteration is done.
                return loopIterationFunc();
            });
            if (iterationCompleted)
            {
                break;
            }
        } while (time.Elapsed() < millisecondsTimeBox);
        return iterationCompleted;
    }

private:
    template<class Func>
    bool IterateCommon(LoopIndexType bound, Func executeLoopIterationAndCheckIfShouldIncrement)
    {
        if (mIsFirstIteration || !Done())
        {
            if (mIsFirstIteration)
            {
                mEndRange = bound;
                mIsFirstIteration = false;
            }

            if (!Done())
            {
                if (executeLoopIterationAndCheckIfShouldIncrement())
                {
                    Increment();
                }
            }
            return false;
        }
        else
        {
            mCurrentIndex = LoopIndexType{};
            mIsFirstIteration = true;
            return true;
        }
    }

    bool Done() const
    {
        return mCurrentIndex >= mEndRange;
    }

    LoopIndexType Increment()
    {
        if (!Done())
        {
            return mCurrentIndex++;
        }
        return LoopIndexType{};
    }

    LoopIndexType mEndRange = {};
    LoopIndexType mCurrentIndex = {};
    bool mIsFirstIteration = true;
};

using IterativeForLoopU32 = IterativeForLoop<u32>;
