#include <gmock/gmock.h>
#include <algorithm>
#include "gridmap.hpp"

class TestCommand : public ICommandWithId<TestCommand>
{
public:
    enum class eOperation
    {
        eAdd,
        eRemove
    };

    TestCommand(std::set<s32>& data, s32 value, eOperation operation) 
        : mData(data), mValue(value), mOperation(operation) {}

    virtual void Redo() override final
    {
        if (mOperation == eOperation::eAdd)
        {
            mData.insert(mValue);
        }
        else
        {
            mData.erase(mValue);
        }
    }

    virtual void Undo() override final
    {
        if (mOperation == eOperation::eAdd)
        {
            mData.erase(mValue);
        }
        else
        {
            mData.insert(mValue);
        }
    }

    virtual std::string Message() override final
    {
        if (mOperation == eOperation::eAdd)
        {
            return "Add " + std::to_string(mValue);
        }
        else
        {
            return "Remove " + std::to_string(mValue);
        }
    }
private:
    std::set<s32>& mData;
    s32 mValue;
    eOperation mOperation;
};

static bool SetsEqual(const std::set<s32>& rhs, const std::set<s32>& lhs)
{
    if (rhs.size() != lhs.size())
    {
        return false;
    }

    for (s32 v : rhs)
    {
        if (lhs.find(v) == std::end(lhs))
        {
            return false;
        }
    }

    return true;
}

TEST(UndoStack, UndoRedo)
{
    std::set<s32> data;

    UndoStack stack;
    ASSERT_TRUE(SetsEqual(data, {}));

    stack.Push<TestCommand>(data, 7, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, {7}));

    stack.Push<TestCommand>(data, 15, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 7, 15 }));

    stack.Push<TestCommand>(data, 15, TestCommand::eOperation::eRemove);
    ASSERT_TRUE(SetsEqual(data, { 7 }));

    // Undo removal of 15
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 7, 15 }));

    // Undo add of 15
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 7 }));
   
    // Undo add of 7
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, {}));
    
    // Nothing left in stack, should do nothing
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, {}));

    // Redo of add 7
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 7 }));

    // Redo of add 15
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 7, 15 }));

    // Undo add of 15
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 7 }));

    // Create a new command which should cause the last 2 commands of add 15 and remove 15 to be removed
    stack.Push<TestCommand>(data, 99, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 7, 99 }));

    // Undo add of 99
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 7 }));

    // Undo add of 7
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, {}));

    // Redo of add 7
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 7 }));

    // Redo of add 99
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 7, 99 }));
    
    // Should now have no effect
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 7, 99 }));

    // Back to the start
    stack.Undo();
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, {  }));

    stack.Push<TestCommand>(data, 3, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 3 }));
   
    // Should have no effect
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 3 }));

    // Undo add of 3
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, {}));
}

TEST(UndoStack, StackLimit)
{
    std::set<s32> data;

    UndoStack stack(2);
    ASSERT_TRUE(SetsEqual(data, {}));

    stack.Push<TestCommand>(data, 1, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 1 }));

    stack.Push<TestCommand>(data, 2, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 1, 2 }));

    stack.Push<TestCommand>(data, 3, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 1, 2, 3 }));

    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1, 2 }));

    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1 }));

    // 1 can't be undone as command 3 will have deleted 1 since the stack limit is 2
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1 }));

    stack.Redo();
    stack.Redo();
    ASSERT_TRUE(SetsEqual(data, { 1, 2, 3 }));

    stack.Push<TestCommand>(data, 4, TestCommand::eOperation::eAdd);
    ASSERT_TRUE(SetsEqual(data, { 1, 2, 3, 4 }));

    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1, 2, 3 }));

    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1, 2 }));

    // 2 can't be undone as command 4 will have deleted 2 since the stack limit is 2
    stack.Undo();
    ASSERT_TRUE(SetsEqual(data, { 1, 2 }));

}
