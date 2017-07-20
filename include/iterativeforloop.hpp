#pragma once

#include "types.hpp"
#include <functional>

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
    bool IterateNested(LoopIndexType bound, Func loopIterationFunc)
    {
        return IterateCommon(bound, [&]()
        {
            // The caller supplied loop function determines if we increment, typically
            // this is the result of another Iterate, i.e the outer loop only increments
            // if the inner loop iteration is done.
            return loopIterationFunc();
        });
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
