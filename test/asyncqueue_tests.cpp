#include <gmock/gmock.h>
#include "asyncqueue.hpp"

class TestJob
{
public:
    void OnExecute(std::atomic_bool&)
    {

    }

    void OnFinished()
    {
        mNumComplete++;
    }

    static int mNumComplete;
};

int TestJob::mNumComplete = 0;

TEST(ASyncQueue, QueueWork)
{
    std::mutex outputMutex;
    ASyncQueue<TestJob> q;
    q.Start();

    for (int i = 0; i < 100; i++)
    {
        q.Add(TestJob{});
    }

    while (!q.IsIdle()) {};

    q.Update();

    ASSERT_EQ(TestJob::mNumComplete, 100);
}
