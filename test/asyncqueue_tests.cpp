#include <gmock/gmock.h>
#include "asyncqueue.hpp"

TEST(ASyncQueue, QueueWork)
{
    ASyncQueue<int> q(4);
    q.QueueWork(std::make_unique<int>(7));
}
