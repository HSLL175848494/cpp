#ifndef HSLL_LOCKFREE_WAITER
#define HSLL_LOCKFREE_WAITER

#include <atomic>

#ifndef Constructor_Delete(cName)
#define Constructor_Delete(cName)\
	cName(const cName&) = delete;\
	cName& operator=(const cName&) = delete;\
	cName(cName&&) = delete;\
	cName& operator=(cName&&) = delete;
#endif

#if defined(_WIN32)
#include <Windows.h>

#define HSLL_HSEM HANDLE

#define HSLL_CREATE_SEMAPHORE(phSem, initialCount, maxCount) \
    *(phSem) = CreateSemaphoreW(nullptr, initialCount, maxCount, nullptr)

#define HSLL_WAIT_SEMAPHORE_INDEFINITE(hSem) \
    WaitForSingleObject(hSem, INFINITE)

#define HSLL_WAIT_SEMAPHORE_TIMEOUT(hSem, timeoutMillis) \
    WaitForSingleObject(hSem, timeoutMillis)

#define HSLL_TRY_WAIT_SEMAPHORE(hSem) \
    (WaitForSingleObject(hSem, 0) == WAIT_OBJECT_0)

#define HSLL_RELEASE_SEMAPHORE(hSem, count) \
    ReleaseSemaphore(hSem, count, nullptr)

#define HSLL_DESTROY_SEMAPHORE(phSem) \
    CloseHandle(*phSem)

#elif defined(__linux__)
#include <semaphore.h>
#include <errno.h>
#include <time.h>

#define HSLL_HSEM sem_t

#define HSLL_CREATE_SEMAPHORE(phSem, initialCount, maxCount) \
    sem_init(phSem, 0, initialCount)

#define HSLL_WAIT_SEMAPHORE_INDEFINITE(hSem) \
    while (sem_wait(&hSem) == -1 && errno == EINTR)

static inline void timespec_add_msec(struct timespec* ts, long milliseconds)
{
	ts->tv_sec += milliseconds / 1000;
	ts->tv_nsec += (milliseconds % 1000) * 1000000;
	if (ts->tv_nsec >= 1000000000)
	{
		ts->tv_sec++;
		ts->tv_nsec -= 1000000000;
	}
}

#define HSLL_WAIT_SEMAPHORE_TIMEOUT(hSem, timeoutMillis)          \
    do                                                            \
    {                                                             \
        struct timespec ts;                                       \
        clock_gettime(CLOCK_REALTIME, &ts);                       \
        timespec_add_msec(&ts, timeoutMillis);                    \
        while (sem_timedwait(&hSem, &ts) == -1 && errno == EINTR) \
            ;                                                     \
    } while (0)

#define HSLL_TRY_WAIT_SEMAPHORE(hSem) \
    (sem_trywait(&hSem) == 0)

#define HSLL_RELEASE_SEMAPHORE(hSem, count) \
    for (int i = 0; i < count; ++i)         \
    sem_post(&hSem)

#define HSLL_DESTROY_SEMAPHORE(phSem) \
    sem_destroy(phSem)

#endif

namespace HSLL
{

	class AtomicLock
	{
		std::atomic<bool> flag;

	public:

		AtomicLock()
		{
			flag.store(true, std::memory_order_relaxed);
		}

		void lock()
		{
			bool old;
			while (true)
			{
				old = true;
				if (flag.compare_exchange_weak(old, false, std::memory_order_relaxed, std::memory_order_relaxed))
					return;
			}
		}

		void unlock()
		{
			flag.store(true, std::memory_order_relaxed);
		}

		Constructor_Delete(AtomicLock);
	};

	class LockFreeWaiter
	{
		HSLL_HSEM hSem;
		AtomicLock lock;
		std::atomic<unsigned int> waitCount;

	public:
		LockFreeWaiter()
		{
			waitCount.store(0, std::memory_order_relaxed);
			HSLL_CREATE_SEMAPHORE(&hSem, 0, 0x7fffffff);
		}

		LockFreeWaiter(unsigned int initialCount, unsigned int maxCount)
		{
			waitCount.store(0, std::memory_order_relaxed);
			HSLL_CREATE_SEMAPHORE(&hSem, initialCount, maxCount);
		}

		void wait()
		{
			if (HSLL_TRY_WAIT_SEMAPHORE(hSem))
				return;
			waitCount.fetch_add(1, std::memory_order_relaxed);
			HSLL_WAIT_SEMAPHORE_INDEFINITE(hSem);
		}

		void wait_for(unsigned int milliseconds)
		{
			if (HSLL_TRY_WAIT_SEMAPHORE(hSem))
				return;
			waitCount.fetch_add(1, std::memory_order_relaxed);
			HSLL_WAIT_SEMAPHORE_TIMEOUT(hSem, milliseconds);
		}

		void notify_one()//由于 wait_for的存在可能信号多发,但专用函数,这无关紧要
		{
			lock.lock();
			if (waitCount.load(std::memory_order_relaxed))
			{
				waitCount.fetch_sub(1, std::memory_order_relaxed);
				lock.unlock();
				HSLL_RELEASE_SEMAPHORE(hSem, 1);
			}
			else
				lock.unlock();
		}

		void notify_all()//由于 wait_for的存在可能信号多发,但专用函数,这无关紧要
		{
			lock.lock();
			unsigned int count = waitCount.load(std::memory_order_relaxed);
			if (count)
			{
				waitCount.fetch_sub(count, std::memory_order_relaxed);
				lock.unlock();
				HSLL_RELEASE_SEMAPHORE(hSem, count);
			}
			else
				lock.unlock();

		}

		~LockFreeWaiter()
		{
			HSLL_DESTROY_SEMAPHORE(&hSem);
		}
		
		Constructor_Delete(LockFreeWaiter);
	};
}
#endif //