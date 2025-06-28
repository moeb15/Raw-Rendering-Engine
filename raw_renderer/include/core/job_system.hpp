#pragma once

#include "core/defines.hpp"
#include <functional>

// implementation from https://wickedengine.net/2018/11/simple-job-system-using-standard-c/
namespace Raw::JobSystem
{
    struct JobDispatchArgs
    {
        u32 jobIndex;
        u32 groupIndex;
    };

    // Create the internal resources such as worker threads, etc. Called once during engine intiailization.
    void Init();

    // Add a job to exectue asynchronously. Any idle thread will execute this job. 
    void Execute(const std::function<void()>& job);

    /**
     * Divide a job into multiple jobs and execute in parallel.
     * @param jobCount : how many jobs to generate for this task.
     * @param groupSize : how many jobs to execute per thread. Jobs inside a group execute serially.
     * @param job : receives a JobDispatchArgs as a parameter.
     */
    void Dispatch(u32 jobCount, u32 groupSize, const std::function<void(JobDispatchArgs)>& job);

    // Check if any threads are working currently or not.
    bool IsBusy();

    // Wait until all threads become idle.
    void Wait();

    u32 GetNumThreads();
    u32 GetCurrentThread();
}