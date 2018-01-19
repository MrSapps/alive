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

template<typename T>
inline T* AsPointer(T& obj)
{
    return &obj;
}

template<typename T>
inline T* AsPointer(T* obj)
{
    return obj; 
}

template<typename T>
inline T* AsPointer(std::unique_ptr<T>& obj)
{
    return obj.get();
}

template<typename T>
inline T* AsPointer(std::shared_ptr<T>& obj)
{
    return obj.get();
}

template<class QueuedItemType>
class ASyncQueue
{
    static_assert(std::is_move_constructible<QueuedItemType>::value == true, "QueuedItemType must be move constructible");
    static_assert(std::is_move_assignable<QueuedItemType>::value == true, "QueuedItemType must be move assignable");
public:
    ASyncQueue()
    {

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
                mWorkQueue.push_back(std::move(item));
            } // Do not hold lock while doing notify_one()
            mHaveWork.notify_one();
        }
    }

    bool IsIdle() const
    {
        std::unique_lock<std::mutex> queueLock(mQueueMutex);
        return mExecutingJobCount == 0 && mWorkQueue.empty();
    }

    // Don't take anymore work, stop any existing work and return immediately while this happens
    void PauseAndCancelASync()
    {
        std::unique_lock<std::mutex> lock(mStartStopMutex);

        {
            std::unique_lock<std::mutex> queueLock(mQueueMutex);
            mWorkQueue.clear();
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
    void Start(s32 numWorkers = std::thread::hardware_concurrency() - 1, bool waitForWorkersToStart = true)
    {
        std::unique_lock<std::mutex> lock(mStartStopMutex);

        assert(mWorkers.empty() == true);

        // Ensure we have at least 1 worker
        if (numWorkers <= 0)
        {
            numWorkers = 1;
        }

        // Create workers
        mWorkers.reserve(numWorkers);
        for (s32 i = 0; i < numWorkers; i++)
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
            mWorkQueue.clear();
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

    // Notify and remove completed work
    void Update()
    {
        std::deque<QueuedItemType> completed;
        {
            // A quick move to avoid holding the lock for too long
            std::unique_lock<std::mutex> completedWorkLock(mCompletedWorkMutex);
            completed = std::move(mCompletedWork);
        }

        for (auto& complete : completed)
        {
            AsPointer(complete)->OnFinished();
        }
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
                return !mWorkQueue.empty() || mQuit;
            });

            if (mQuit)
            {
                // In this case outstanding jobs are not notified of completion
                return;
            }

            if (!mWorkQueue.empty())
            {
                auto item = std::move(mWorkQueue.front());
                mWorkQueue.pop_front();

                lock.unlock();

                mExecutingJobCount++;
                AsPointer(item)->OnExecute(mStopWork);
                mExecutingJobCount--;

                // Notify call back on main thread of completed work
                {
                    std::unique_lock<std::mutex> completedWorkLock(mCompletedWorkMutex);
                    mCompletedWork.push_back(std::move(item));
                }

                lock.lock();
            }
        }
    }

    std::mutex mStartStopMutex; // Prevent concurrent Start/Stop/Add
    mutable std::mutex mQueueMutex;     // Protect mWorkQueue
    mutable std::mutex mCompletedWorkMutex;     // Protect mCompletedWork
    std::atomic_bool mQuit { false };
    std::atomic_bool mStopWork { false };
    std::atomic_int mRunningThreadCount { 0 };
    std::atomic_uint mExecutingJobCount { 0 };
    std::condition_variable mHaveWork;
    std::deque<QueuedItemType> mWorkQueue;
    std::deque<QueuedItemType> mCompletedWork;
    std::vector<std::thread> mWorkers;
};
