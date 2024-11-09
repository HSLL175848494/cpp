#ifndef HSLL_THREADPOOL
#define HSLL_THREADPOOL

#include <vector>
#include <thread>
#include <atomic>
#include "../ConcurrentQueue/blockingconcurrentqueue.h"

namespace HSLL
{

#define TRY_ENQUEUE_SIZE 100

	template <class T>
	class ThreadPool
	{
	private:
		bool flag;
		std::atomic<int> nowSize;
		std::vector<std::thread> threads;
		moodycamel::BlockingConcurrentQueue<T> taskQueue;

	public:
		ThreadPool() : flag(false) {}

		template <typename U>
		bool Append(U &&task)
		{
			if (nowSize.load(std::memory_order_relaxed) > 0)
			{
				bool ret = taskQueue.enqueue(std::forward<U>(task));
				if (ret)
					nowSize.fetch_sub(1, std::memory_order_relaxed);
				return ret;
			}
			else
				return false;
		}

		unsigned int AppendSeveral(T *tasks, unsigned int num) // 返回实际append数目
		{
			if (nowSize.load(std::memory_order_relaxed) > 0)
			{
				unsigned int rNum = taskQueue.enqueue_bulk(tasks, num);
				if (rNum)
					nowSize.fetch_sub(rNum, std::memory_order_relaxed);
				return rNum;
			}
			else
				return 0;
		}

		void workerSeveral()
		{
			moodycamel::ConsumerToken t(taskQueue);
			char tasksSpace[sizeof(T) * TRY_ENQUEUE_SIZE];
			T *tasks = (T *)tasksSpace; // 转化指针避免T的构造
			while (true)
			{
				unsigned int num = taskQueue.wait_dequeue_bulk(t, tasks, TRY_ENQUEUE_SIZE);
				if (flag)
				{
					nowSize.fetch_add(num, std::memory_order_relaxed);
					for (unsigned int i = 0; i < num; i++)
						tasks[i].execute();
					continue;
				}
				return;
			}
		}

		void workerSingle()
		{
			moodycamel::ConsumerToken t(taskQueue);
			char tasksSpace[sizeof(T)];
			T *tasks = (T *)tasksSpace; // 转化指针避免T的构造
			while (true)
			{
				taskQueue.wait_dequeue(t, tasks[0]);
				if (flag)
				{
					nowSize.fetch_add(1, std::memory_order_relaxed);
					tasks[0].execute();
					continue;
				}
				return;
			}
		}

		bool Start(int queueLength = 10000, unsigned int threadNum = 6) // 开启线程
		{
			if (threadNum)
			{
				flag = true;
				nowSize.store(queueLength, std::memory_order_relaxed);

				std::thread thread(&ThreadPool::workerSingle, this);
				threads.push_back(std::move(thread));
				for (unsigned int i = 1; i < threadNum; i++)
					threads.emplace_back(&ThreadPool::workerSeveral, this);
				return true;
			}
			return false;
		}

		void Release() // 释放线程，此时应该已经停止所有append操作
		{
			flag = false;
			unsigned int size = TRY_ENQUEUE_SIZE * threads.size() - TRY_ENQUEUE_SIZE + 1;
			T *tasksNull = (T *)(new char[sizeof(T) * size]); // 确保能唤醒所有线程
			taskQueue.enqueue_bulk(tasksNull, size);
			for (unsigned int i = 0; i < threads.size(); i++)
				threads[i].join();
			delete[] tasksNull;
		}

		~ThreadPool()
		{
			if (flag)
			Release();
		}
	};
}

#endif