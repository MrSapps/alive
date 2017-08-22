#pragma once

#include <memory>
#include <vector>
#include <string>
#include <map>
#include <mutex>
#include <deque>
#include <future>
#include "types.hpp"
#include <assert.h>
#include "logger.hpp"

template<class QueuedItemType>
class ASyncQueue
{
    static_assert(std::is_move_constructible<QueuedItemType>::value == true, "QueuedItemType must be move constructible");
    static_assert(std::is_move_assignable<QueuedItemType>::value == true, "QueuedItemType must be move assignable");
public:
    ASyncQueue(std::function<void(QueuedItemType item, std::atomic<bool>& quitFlag)> executeItemNoLock)
        : mExecFunc(executeItemNoLock)
    {
        assert(mExecFunc != nullptr);
    }

    ~ASyncQueue()
    {
        Stop();
    }

    void Add(QueuedItemType item)
    {
        if (!mQuit && !mStopWork)
        {
            {
                std::unique_lock<std::mutex> startStopLock(mStartStopMutex);
                std::unique_lock<std::mutex> queueLock(mQueueMutex);
                mQueue.push_back(std::move(item));
            } // Do not hold lock while doing notify_one()
            mHaveWork.notify_one();
        }
    }

    bool IsIdle() const
    {
        std::unique_lock<std::mutex> queueLock(mQueueMutex);
        return mExecutingJobCount == 0 && mQueue.empty();
    }

    // Don't take anymore work, stop any existing work and return immediately while this happens
    void PauseAndCancelASync()
    {
        std::unique_lock<std::mutex> lock(mStartStopMutex);
        
        {
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            mQueue.clear();
        }

        mStopWork = true;
        mHaveWork.notify_all();
    }

    // Start taking work again
    void UnPause()
    {
        mStopWork = false;
    }

    // Default to half of the CPU cores - if we use all of them then it will take too much time from
    // whatever core is running the main thread/game loop.
    void Start(u32 numWorkers = std::thread::hardware_concurrency() / 2, bool waitForWorkersToStart = true)
    {
        assert(mWorkers.empty() == true);

        std::unique_lock<std::mutex> lock(mStartStopMutex);

        // Ensure we have at least 1 worker
        if (numWorkers == 0)
        {
            numWorkers = 1;
        }

        // Create workers
        for (u32 i = 0; i < numWorkers; i++)
        {
            mWorkers.push_back(std::thread(&ASyncQueue::WorkerFunc, this));
        }

        if (waitForWorkersToStart)
        {
            while (mRunningThreadCount < numWorkers) {}
        }

        LOG_INFO("Worker queue is using: " << numWorkers << " workers");
    }

    void Stop()
    {
        std::unique_lock<std::mutex> lock(mStartStopMutex);

        mQuit = true;
        mStopWork = true;

        {
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            mQueue.clear();
        }

        mHaveWork.notify_all();

        // Wait for running workers to exit
        for (auto& thread : mWorkers)
        {
            if (thread.joinable())
            {
                thread.join();
            }
        }

        mWorkers.clear();
        mRunningThreadCount = 0;
    }

private:
    void WorkerFunc()
    {
        mRunningThreadCount++;

        std::unique_lock<std::mutex> lock(mQueueMutex);
        while (!mQuit)
        {
            mHaveWork.wait(lock, [this]() 
            {
                return !mQueue.empty() || mQuit;
            });

            if (mQuit)
            {
                return;
            }

            if (!mQueue.empty())
            {
                auto item = std::move(mQueue.front());
                mQueue.pop_front();

                lock.unlock();

                mExecutingJobCount++;
                mExecFunc(std::move(item), mStopWork);
                mExecutingJobCount--;

                lock.lock();
            }
        }
    }

    std::mutex mStartStopMutex; // Prevent concurrent Start/Stop/Add
    mutable std::mutex mQueueMutex;     // Protect mQueue
    std::atomic<bool> mQuit { false };
    std::atomic<bool> mStopWork { false };
    std::atomic<u32> mRunningThreadCount { 0 };
    std::atomic<u32> mExecutingJobCount { 0 };
    std::condition_variable mHaveWork;
    std::deque<QueuedItemType> mQueue;
    std::vector<std::thread> mWorkers;
    std::function<void(QueuedItemType item, std::atomic<bool>& quitFlag)> mExecFunc;
};
