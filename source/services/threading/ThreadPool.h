#pragma once
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "services/Service.h"

namespace parus
{
    
    class ThreadPool final : public Service
    {
    public:
        ThreadPool() = default;
        ~ThreadPool();
        ThreadPool(const ThreadPool&) = delete;
        ThreadPool(ThreadPool&&) = delete;
        ThreadPool& operator=(const ThreadPool&) = delete;
        ThreadPool& operator=(const ThreadPool&&) = delete;

        static unsigned int defaultThreadCount();

        void init(unsigned int numberOfThreads = defaultThreadCount());
        void enqueue(std::function<void()> task);
        void waitUntilDone();

        [[nodiscard]] bool isBusy() const;
        
    private:
        void workerJob();
        
        std::vector<std::thread> workers;
        std::queue<std::function<void()>> tasks;
        mutable std::mutex queueMutex;
        std::condition_variable conditionVariable;
        std::condition_variable completionVariable;
        unsigned int activeTasks = 0;
        bool isPendingStop = false;
    };

#define RUN_ASYNC(function) Services::get<ThreadPool>()->enqueue([=]{function})
    
}
