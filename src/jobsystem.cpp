#include "jobsystem.hpp"
#include "asyncqueue.hpp"

JobSystem::JobSystem()
{
    TRACE_ENTRYEXIT;
    mAsyncQueue = std::make_unique<ASyncQueue_SP_IJob>();
    mAsyncQueue->Start();
}

JobSystem::~JobSystem()
{
    TRACE_ENTRYEXIT;
    mAsyncQueue->Stop();
    mAsyncQueue = nullptr;
}

SP_IJob JobSystem::StartJob(SP_IJob job)
{
    mAsyncQueue->Add(job);
    return job;
}

void JobSystem::Update()
{
    mAsyncQueue->Update();
}

SP_IJob JobTracker::StartJob(SP_IJob job)
{
    auto trackedJob = std::make_shared<TrackedJobWrapper>(*this, job);
    mOutstandingJobs.insert(trackedJob);
    return mJobSystem.StartJob(trackedJob);
}

TrackedJobWrapper::TrackedJobWrapper(JobTracker& jobTracker, SP_IJob trackedJob) 
    : mJobTracker(jobTracker), mTrackedJob(trackedJob)
{

}
