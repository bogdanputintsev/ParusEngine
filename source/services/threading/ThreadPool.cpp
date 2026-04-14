#include "ThreadPool.h"
#include "engine/EngineCore.h"

namespace parus
{
    ThreadPool::~ThreadPool()
    {
        {
            std::lock_guard lock(queueMutex);
            isPendingStop = true;
        }

        conditionVariable.notify_all();

        for (auto& worker : workers)
        {
            worker.join();
        }
    }

    void ThreadPool::workerJob()
    {
        while (true)
        {
            std::function<void()> newTask;
            
            {
                std::unique_lock lock(queueMutex);
                conditionVariable.wait(lock, [&]()
                {
                   return isPendingStop || !tasks.empty(); 
                });

                // Exit condition.
                if (isPendingStop && tasks.empty())
                {
                    return;
                }

                newTask = std::move(tasks.front());
                tasks.pop();
                ++activeTasks;
            }

            newTask();

            {
                std::scoped_lock completionLock(queueMutex);
                --activeTasks;
            }
            completionVariable.notify_all();
        }
    }

    unsigned int ThreadPool::defaultThreadCount()
    {
        const unsigned int cores = std::thread::hardware_concurrency();
        return cores > 1 ? cores - 1 : 1;
    }

    void ThreadPool::init(const unsigned int numberOfThreads)
    {
        LOG_INFO("Initializing Thread Pool with " + std::to_string(numberOfThreads) + " threads.");
        for (unsigned int i = 0; i < numberOfThreads; ++i)
        {
            workers.emplace_back(&ThreadPool::workerJob, this);
        }
    }

    void ThreadPool::enqueue(std::function<void()> task)
    {
        {
            std::scoped_lock lock(queueMutex);
            tasks.emplace(std::move(task));
        }
        conditionVariable.notify_all();
    }

    void ThreadPool::waitUntilDone()
    {
        std::unique_lock lock(queueMutex);
        completionVariable.wait(lock, [&]()
        {
            return tasks.empty() && activeTasks == 0;
        });
    }

    bool ThreadPool::isBusy() const
    {
        std::scoped_lock lock(queueMutex);
        return !tasks.empty() || activeTasks > 0;
    }
}