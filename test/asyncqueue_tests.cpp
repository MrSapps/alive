#include <gmock/gmock.h>
#include "asyncqueue.hpp"

TEST(ASyncQueue, QueueWork)
{
    std::mutex outputMutex;
    ASyncQueue<std::unique_ptr<int>> q([&](std::unique_ptr<int> v, std::atomic<bool>&) 
    {
        std::unique_lock<std::mutex> lock(outputMutex);
        std::cout << "V: " << *v << std::endl;
    });
    q.Start();

    for (int i = 0; i < 100; i++)
    {
        q.Add(std::make_unique<int>(i));
    }
}
