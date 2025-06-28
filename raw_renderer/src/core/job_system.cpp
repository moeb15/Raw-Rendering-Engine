#include "core/job_system.hpp"
#include "containers/thread_safe_ring_buffer.hpp"
#include "core/timer.hpp"
#include <algorithm>
#include <atomic>
#include <thread>
#include <condition_variable>
#include <unordered_map>

namespace Raw::JobSystem
{
    u32 numThreads = 0;
    ThreadSafeRingBuffer<std::function<void()>, 256> jobPool;
    std::condition_variable wakeCondition;
    std::mutex wakeMutex;
    u64 curLabel = 0; // tarackes the state of execution of the main thread
    std::atomic<u64> finishedLabel; // track the state of execution across background worker threads
    std::unordered_map<u64, u32> idToNum;

    void Init()
    {
        finishedLabel.store(0);
        u32 numCores = std::thread::hardware_concurrency();
        numThreads = std::max(1u, numCores);

        // create all worker threads while immediately starting them
        for(u32 threadId = 0; threadId < numThreads; threadId++)
        {
            std::thread worker([]
                {
                    std::function<void()> job; // the current job for the thread, starts out empty

                    // this is the infinite loop that a worker thread will execute
                    while(true)
                    {
                        if(jobPool.Pop(job)) // try to grab a job from the job pool
                        {
                            job(); // execute job
                            finishedLabel.fetch_add(1); // update worker label state
                        }
                        else
                        {
                            // no job, put thread to sleep
                            std::unique_lock<std::mutex> lock(wakeMutex);
                            wakeCondition.wait(lock);
                        }
                    }
                }
            );

            idToNum[std::hash<std::thread::id>{}(worker.get_id())] = threadId;
            worker.detach(); // forget about this thread, let it do its job in the infinite loop that we created above
        }
    }

    // avoids deadlock while the main thread is waiting for something
    void Poll()
    {
        wakeCondition.notify_one(); // wake one worker thread
        std::this_thread::yield(); // allows this thread to be rescheduled
    }

    void Execute(const std::function<void()>& job)
    {
        // the main thread lable state is updated
        curLabel += 1;

        // type to push the new job util it is pushed successfully
        while(!jobPool.Push(job)) { Poll(); }

        wakeCondition.notify_one(); // wake one thread
    }

    bool IsBusy()
    {
        // whenever the main thread label is not reached by the workers, it indicates that some worker is still alive
        return finishedLabel.load() < curLabel;
    }

    void Wait()
    {
        while(IsBusy()) { Poll(); }
    }

    void Dispatch(u32 jobCount, u32 groupSize, const std::function<void(JobDispatchArgs)>& job)
    {
        if(jobCount == 0 || groupSize == 0) return;

        // calculate the amount of job groups to dispatch
        const u32 groupCount = (jobCount + groupSize - 1) / groupSize;

        // the main thread lable state is updated
        curLabel += groupCount;

        for(u32 groupIndex = 0; groupIndex < groupCount; groupIndex++)
        {
            // for each group, generate one real job
            auto jobGroup = [jobCount, groupSize, job, groupIndex]()
            {
                // calculate the current groups offset into the jobs
                const u32 groupJobOffset = groupIndex * groupSize;
                const u32 groupJobEnd = std::min(groupJobOffset + groupSize, jobCount);
                
                JobDispatchArgs args;
                args.groupIndex = groupIndex;
                
                // inside the group, loop through all the job indices and execute job for each index
                for(u32 i = groupJobOffset; i < groupJobEnd; i++)
                {
                    args.jobIndex = i;
                    job(args);
                }
            };

            // try to push a new job until it is pushed successfully
            while(!jobPool.Push(jobGroup)) { Poll(); }

            wakeCondition.notify_one();
        }
    }

    u32 GetNumThreads()
    {
        return numThreads;
    }
    
    u32 GetCurrentThread()
    {
        return idToNum[std::hash<std::thread::id>{}(std::this_thread::get_id())];
    }
}