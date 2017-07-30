#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <future>
#include "types.hpp"

template<class T>
class ASyncQueue
{
public:
    ASyncQueue(u32 numWorkers)
    {
        assert(numWorkers >= 1);

        for (u32 i = 0; i < numWorkers; i++)
        {
            mWorkers.push_back(std::thread(&ASyncQueue::WorkerFunc, this));
        }
    }

    ~ASyncQueue()
    {
        Stop();
    }

    void Cancel()
    {

    }

    void QueueWork(std::unique_ptr<T> item)
    {
        std::unique_lock<std::mutex> lock(mMutex);
        mQueue.push_back(std::move(item));
    }

    bool IsIdle()
    {
        return false;
    }

    void Stop()
    {

    }

private:
    void WorkerFunc()
    {
        std::unique_lock<std::mutex> lock;
        mHaveWork.wait(lock);
    }

    std::mutex mMutex;
    std::condition_variable mHaveWork;
    std::deque<std::unique_ptr<T>> mQueue;
    std::deque<std::unique_ptr<T>> mExecutingItems;
    std::vector<std::thread> mWorkers;
};
