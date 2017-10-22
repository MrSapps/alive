#pragma once

#include <atomic>
#include <memory>
#include <set>
#include "types.hpp"

class CancelFlag
{
public:
    CancelFlag(std::atomic_bool& stopASyncQueueFlag, std::atomic_bool& cancelSpecificJobFlag)
        : mStopASyncQueueFlag(stopASyncQueueFlag), mCancelSpecificJobFlag(cancelSpecificJobFlag)
    {

    }

    bool IsCancelled() const
    {
        return mStopASyncQueueFlag || mCancelSpecificJobFlag;
    }

private:
    std::atomic_bool& mStopASyncQueueFlag;
    std::atomic_bool& mCancelSpecificJobFlag;
};

class IJob
{
public:
    virtual ~IJob() = default;
    
    void OnExecute(std::atomic_bool& quitFlag)
    {
        CancelFlag cancelFlag(mCancel, quitFlag);
        OnExecute(cancelFlag);
    }

    virtual void OnExecute(const CancelFlag& quitFlag) = 0;
    virtual void OnFinished() = 0;

    virtual void Cancel()
    {
        mCancel = true;
    }

private:
    std::atomic_bool mCancel{ false };
};

using SP_IJob = std::shared_ptr<IJob>;

template<class T>
class ASyncQueue;

using ASyncQueue_SP_IJob = ASyncQueue<SP_IJob>;

class JobSystem
{
public:
    JobSystem();
    ~JobSystem();
    SP_IJob StartJob(SP_IJob job);
    void Update();
private:
    // unique_ptr to keep ASyncQueue header out of this header
    std::unique_ptr<ASyncQueue_SP_IJob> mAsyncQueue;
};

class JobTracker
{
public:
    JobTracker(JobSystem& jobSystem)
        : mJobSystem(jobSystem)
    {

    }

    SP_IJob StartJob(SP_IJob job);

    void CancelOutstandingJobs()
    {
        for (auto& job : mOutstandingJobs)
        {
            job->Cancel();
        }
    }

    u32 OutstandingJobCount() const
    {
        return static_cast<u32>(mOutstandingJobs.size());
    }

    void OnFinished(SP_IJob job)
    {
        mOutstandingJobs.erase(job);
    }

private:
    JobSystem& mJobSystem;
    std::set<SP_IJob> mOutstandingJobs;
};

class TrackedJobWrapper : public IJob, public std::enable_shared_from_this<TrackedJobWrapper>
{
public:
    TrackedJobWrapper(JobTracker& jobTracker, SP_IJob trackedJob);

    virtual void OnExecute(const CancelFlag& quitFlag) override
    {
        mTrackedJob->OnExecute(quitFlag);
    }

    virtual void OnFinished() override
    {
        mJobTracker.OnFinished(shared_from_this());
        mTrackedJob->OnFinished();
    }

    virtual void Cancel() override
    {
        mTrackedJob->Cancel();
        IJob::Cancel(); // Because CancelFlag contains this->mCancel, not mTrackedJob->mCancel
    }
private:
    JobTracker& mJobTracker;
    SP_IJob mTrackedJob;
};
