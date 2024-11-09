#ifndef HSLL_THREADPOOL
#define HSLL_THREADPOOL

#include <thread>
#include<atomic>
#include "CoStruct.hpp"

namespace HSLL
{

    class TaskBase
    {
        typedef void(*StaticFreeFunc)(TaskBase*);

    public:

        StaticFreeFunc func;

        TaskBase(StaticFreeFunc func) :func(func) {};

        virtual void execute() = 0;
    };

    class ThreadPool
    {
    private:

        bool flag = false;
        std::atomic<unsigned int> exitNum;
        CoStruct<Queue, TaskBase> queueTask;

    public:

        bool Append(TaskBase* task)
        {
            return queueTask.Push(task);
        }

        void worker()
        {
            while (flag)
            {
                TaskBase* pTask = queueTask.WaitPop();
                if (pTask)
                {
                    pTask->execute();
                    pTask->func(pTask);
                }
            }
            exitNum.fetch_sub(1, std::memory_order_relaxed);
        }

        void Init(unsigned int queueMaxLength = 10000, unsigned short threadNum = 8)
        {
            exitNum = threadNum;
            queueTask.Init(queueMaxLength);
            for (unsigned short i = 0; i < threadNum; i++)
            {
                std::thread thread(&ThreadPool::worker, this);
                thread.detach();
            }
        }

        void Release()
        {
            flag = false;
            queueTask.StopWait();
            while (exitNum.load(std::memory_order_relaxed))
            std::this_thread::yield();
            TaskBase* pTask = queueTask.Pop();
            while (pTask)
            {
                pTask->func(pTask);
                pTask = queueTask.Pop();
            }
        }

        ~ThreadPool()
        {
            if (flag)
                Release();
        }

        ThreadPool() : flag(true) {}
    };
}

#endif