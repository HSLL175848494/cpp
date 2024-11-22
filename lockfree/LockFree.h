#ifndef HSLL_LOCKFREE
#define HSLL_LOCKFREE

#include<thread>
#include <chrono>
#include <condition_variable>
#include"LockFree_Waiter.h"

namespace HSLL
{

#define LOCKFREE_MALLOC(size) malloc(size)

#define LOCKFREE_FREE(p) free(p)

#define LOCKFREE_FINALIZE \
	New.Free = 1;         \
	Lock.store(New, std::memory_order_release);

#define LOCKFREE_FINALIZE_2(delta_expr) \
	New = {Old.Size + (delta_expr), 1}; \
	Lock.store(New, std::memory_order_release);

#define LOCKFREE_CAS_OP(delta_expr, yield)                                                           \
	Old.Free = 1;                                                                                    \
	New = {Old.Size + (delta_expr), 0};                                                              \
	if (!Lock.compare_exchange_weak(Old, New, std::memory_order_acquire, std::memory_order_relaxed)) \
	{                                                                                                \
		yield;                                                                                       \
		goto Reload;                                                                                 \
	}

#define LOCKFREE_PREPARE                        \
	Param Old;                                  \
	Param New;                                  \
	Resume:                                     \
	Old = Lock.load(std::memory_order_relaxed); \
	Reload:

#define LOCKFREE_PREPARE_RET \
	int ret;                 \
	LOCKFREE_PREPARE

#define LOCKFREE_PREPARE_RET_TIMEOUT \
	unsigned int TimeOut = 1;        \
	LOCKFREE_PREPARE_RET

#define LOCKFREE_PREPARE_TIMEOUT \
	unsigned int TimeOut = 1;    \
	LOCKFREE_PREPARE

#define LOCKFREE_CV(mtx, cv)                       \
	if (!Wait)                                           \
		return 0;                                        \
	std::unique_lock<std::mutex> uLock(mtx);             \
	cv.wait_for(uLock, std::chrono::milliseconds(TimeOut)); \
	if (TimeOut != 0xffffffff)                           \
		TimeOut <<= 1;                                   \
	goto Resume;

#define LOCKFREE_WAITER                      \
	if (!Wait)                                           \
		return 0;                                        \
	waiter.wait_for(TimeOut); \
	if (TimeOut != 0xffffffff)                           \
		TimeOut <<= 1;                                   \
	goto Resume;


#define LOCKFREE_MALLOC_ONE                           \
	Node *NewNode = (Node *)LOCKFREE_MALLOC(sizeof(Node)); \
	NewNode->value = std::forward<Type>(inValue);

#define LOCKFREE_MALLOC_BULK                      \
	Node *NewNode = (Node *)LOCKFREE_MALLOC(sizeof(Node)); \
	NewNode->value = inValues[0];                 \
	Node *Ptr = NewNode;                          \
	for (unsigned int i = 1; i < size; i++)       \
	{                                             \
		Ptr->next = (Node *)LOCKFREE_MALLOC(sizeof(Node)); \
		Ptr = Ptr->next;                          \
		Ptr->value = inValues[i];                 \
	}

#define LOCKFREE_FREE_ONE       \
	reValue = pFree->value; \
	LOCKFREE_FREE(pFree);

#define LOCKFREE_FREE_LIST                  \
	for (unsigned int i = 0; i < ret; i++) \
	{                                      \
		reValues[i] = pFree->value;        \
		Node *temp = pFree;                \
		pFree = pFree->next;               \
		LOCKFREE_FREE(temp);                        \
	}

#define LOCKFREE_STACK_PUSH                    \
	LOCKFREE_CAS_OP(1, )                       \
	Values[Top] = std::forward<Type>(inValue); \
	Top++;                                     \
	LOCKFREE_FINALIZE

#define LOCKFREE_STACK_POP \
	LOCKFREE_CAS_OP(-1, )  \
	Top--;                 \
	reValue = Values[Top]; \
	LOCKFREE_FINALIZE

#define LOCKFREE_STACK_PUSH_BULK                                       \
	ret = (Old.Size + size > MaxLength) ? MaxLength - Old.Size : size; \
	LOCKFREE_CAS_OP(ret, )                                             \
	for (int i = 0; i < ret; i++)                                      \
	{                                                                  \
		Values[Top] = inValues[i];                                     \
		Top++;                                                         \
	}                                                                  \
	LOCKFREE_FINALIZE

#define LOCKFREE_STACK_POP_BULK                \
	ret = (Old.Size > size) ? size : Old.Size; \
	LOCKFREE_CAS_OP(-ret, )                    \
	for (int i = 0; i < ret; ++i)              \
	{                                          \
		Top--;                                 \
		reValues[i] = Values[Top];             \
	}                                          \
	LOCKFREE_FINALIZE

#define LOCKFREE_STACK_PUSH_LIST(yield) \
	LOCKFREE_CAS_OP(0, yield)           \
	NewNode->next = Top;                \
	Top = NewNode;                      \
	LOCKFREE_FINALIZE_2(1);

#define LOCKFREE_STACK_POP_LIST \
	LOCKFREE_CAS_OP(-1, )       \
	Node *pFree = Top;          \
	Top = Top->next;            \
	LOCKFREE_FINALIZE

#define LOCKFREE_STACK_PUSH_BULK_LIST(yield) \
	LOCKFREE_CAS_OP(0, yield)                \
	Ptr->next = Top;                         \
	Top = NewNode;                           \
	LOCKFREE_FINALIZE_2(size);

#define LOCKFREE_STACK_POP_BULK_LIST         \
	ret = Old.Size > size ? size : Old.Size; \
	LOCKFREE_CAS_OP(-ret, )                  \
	Node *pFree = Top;                       \
	for (unsigned int i = 0; i < ret; i++)   \
		Top = Top->next;                     \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_PUSH                  \
	LOCKFREE_CAS_OP(1, )                     \
	Values[Head] = std::forward<T>(inValue); \
	Head = (Head + 1) % MaxLength;           \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_POP         \
	LOCKFREE_CAS_OP(-1, )          \
	reValue = Values[Tail];        \
	Tail = (Tail + 1) % MaxLength; \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_PUSH_BULK                                       \
	ret = (Old.Size + size > MaxLength) ? MaxLength - Old.Size : size; \
	LOCKFREE_CAS_OP(ret, )                                             \
	for (int i = 0; i < ret; i++)                                      \
	{                                                                  \
		Values[Head] = inValues[i];                                    \
		Head = (Head + 1) % MaxLength;                                 \
	}                                                                  \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_POP_BULK                \
	ret = (Old.Size > size) ? size : Old.Size; \
	LOCKFREE_CAS_OP(-ret, )                    \
	for (int i = 0; i < ret; i++)              \
	{                                          \
		reValues[i] = Values[Tail];            \
		Tail = (Tail + 1) % MaxLength;         \
	}                                          \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_PUSH_LIST(yield) \
	LOCKFREE_CAS_OP(0, yield)           \
	if (Head)                           \
		Head->next = NewNode;           \
	else                                \
		Tail = NewNode;                 \
	Head = NewNode;                     \
	LOCKFREE_FINALIZE_2(1)

#define LOCKFREE_QUEUE_POP_LIST \
	LOCKFREE_CAS_OP(-1, )       \
	Node *pFree = Tail;         \
	Tail = Tail->next;          \
	if (!Tail)                  \
		Head = nullptr;         \
	LOCKFREE_FINALIZE

#define LOCKFREE_QUEUE_PUSH_BULK_LIST(yield) \
	LOCKFREE_CAS_OP(0, yield)                \
	if (Head)                                \
		Head->next = NewNode;                \
	else                                     \
		Tail = NewNode;                      \
	Head = Ptr;                              \
	LOCKFREE_FINALIZE_2(size)

#define LOCKFREE_QUEUE_POP_BULK_LIST           \
	ret = (Old.Size > size) ? size : Old.Size; \
	LOCKFREE_CAS_OP(-ret, )                    \
	Node *pFree = Tail;                        \
	for (unsigned int i = 0; i < ret; i++)     \
		Tail = Tail->next;                     \
	if (!Tail)                                 \
		Head = nullptr;                        \
	LOCKFREE_FINALIZE

	template <class T>
	class LockFreeStack_Array
	{
		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		T* Values = nullptr;
		unsigned int Top;
		unsigned int MaxLength;
		std::atomic<Param> Lock;

	public:
		void Init(unsigned int maxLength)
		{
			Top = 0;
			MaxLength = maxLength;
			Values = (T*)malloc(maxLength * sizeof(T));
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		bool Push(Type&& inValue)
		{
			LOCKFREE_PREPARE;
			if (Old.Size == MaxLength)
				return false;
			LOCKFREE_STACK_PUSH;
			return true;
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_STACK_POP;
			return true;
		}

		unsigned int PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (Old.Size == MaxLength)
				return 0;
			LOCKFREE_STACK_PUSH_BULK;
			return ret;
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_STACK_POP_BULK;
			return ret;
		}

		~LockFreeStack_Array()
		{
			if (Values)
				free(Values);
		}
		LockFreeStack_Array() {};
		Constructor_Delete(LockFreeStack_Array);
	};

	template <class T>
	class LockFreeBlockStack_Array
	{
		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		bool Wait;
		T* Values = nullptr;
		unsigned int Top;
		unsigned int MaxLength;
		std::mutex mtxPush;
		std::mutex mtxPop;
		std::atomic<Param> Lock;
		std::condition_variable cvPush;
		std::condition_variable cvPop;

	public:
		void Init(unsigned int maxLength)
		{
			Top = 0;
			Wait = true;
			MaxLength = maxLength;
			Values = (T*)malloc(maxLength * sizeof(T));
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		bool Push(Type&& inValue)
		{
			LOCKFREE_PREPARE;
			if (Old.Size == MaxLength)
				return false;
			LOCKFREE_STACK_PUSH;
			cvPop.notify_one();
			return true;
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_STACK_POP;
			cvPush.notify_one();
			return true;
		}

		unsigned int PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (Old.Size == MaxLength)
				return 0;
			LOCKFREE_STACK_PUSH_BULK;
			cvPop.notify_all();
			return ret;
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_STACK_POP_BULK;
			cvPush.notify_all();
			return ret;
		}

		template <class Type>
		bool WaitPush(Type&& inValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (Old.Size == MaxLength)
			{
				LOCKFREE_CV(mtxPush, cvPush);
			}
			LOCKFREE_STACK_PUSH;
			cvPop.notify_all();
			return true;
		}

		bool WaitPop(T& reValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_CV(mtxPop, cvPop);
			}
			LOCKFREE_STACK_POP;
			cvPush.notify_all();
			return true;
		}

		unsigned int WaitPushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (Old.Size == MaxLength)
			{
				LOCKFREE_CV(mtxPush, cvPush);
			}
			LOCKFREE_STACK_PUSH_BULK;
			cvPop.notify_all();
			return ret;
		}

		unsigned int WaitPopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_CV(mtxPop, cvPop);
			}
			LOCKFREE_STACK_POP_BULK;
			cvPush.notify_all();
			return ret;
		}

		void StopWait()
		{
			Wait = false;
			cvPop.notify_all();
			cvPush.notify_all();
		}

		~LockFreeBlockStack_Array()
		{
			if (Values)
				free(Values);
		}
		LockFreeBlockStack_Array() {};
		Constructor_Delete(LockFreeBlockStack_Array);
	};

	template <class T>
	class LockFreeQueue_Array
	{
		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		T* Values = nullptr;
		unsigned int Head;
		unsigned int Tail;
		unsigned int MaxLength;
		std::atomic<Param> Lock;

	public:
		void Init(unsigned int maxLength)
		{
			MaxLength = maxLength;
			Values = (T*)malloc(maxLength * sizeof(T));
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		bool Push(Type&& inValue)
		{
			LOCKFREE_PREPARE;
			if (Old.Size == MaxLength)
				return false;
			LOCKFREE_QUEUE_PUSH;
			return true;
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_QUEUE_POP;
			return true;
		}

		unsigned int PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (Old.Size == MaxLength)
				return 0;
			LOCKFREE_QUEUE_PUSH_BULK;
			return ret;
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_QUEUE_POP_BULK;
			return ret;
		}

		~LockFreeQueue_Array()
		{
			if (Values)
				free(Values);
		}
		LockFreeQueue_Array() {};
		Constructor_Delete(LockFreeQueue_Array);
	};

	template <class T>
	class LockFreeBlockQueue_Array
	{
		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		bool Wait;
		T* Values = nullptr;
		unsigned int Head;
		unsigned int Tail;
		unsigned int MaxLength;
		std::mutex mtxPop;
		std::mutex mtxPush;
		std::atomic<Param> Lock;
		std::condition_variable cvPop;
		std::condition_variable cvPush;

	public:
		void Init(unsigned int maxLength)
		{
			Wait = true;
			MaxLength = maxLength;
			Values = (T*)malloc(maxLength * sizeof(T));
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		bool Push(Type&& inValue)
		{
			LOCKFREE_PREPARE;
			if (Old.Size == MaxLength)
				return false;
			LOCKFREE_QUEUE_PUSH;
			cvPop.notify_one();
			return true;
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_QUEUE_POP;
			cvPush.notify_one();
			return true;
		}

		unsigned int PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (Old.Size == MaxLength)
				return 0;
			LOCKFREE_QUEUE_PUSH_BULK;
			cvPop.notify_all();
			return ret;
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_QUEUE_POP_BULK;
			cvPush.notify_all();
			return ret;
		}

		template <class Type>
		bool WaitPush(Type&& inValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (Old.Size == MaxLength)
			{
				LOCKFREE_CV(mtxPush, cvPush);
			}
			LOCKFREE_QUEUE_PUSH;
			cvPop.notify_all();
			return true;
		}

		bool WaitPop(T& reValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_CV(mtxPop, cvPop);
			}
			LOCKFREE_QUEUE_POP;
			cvPush.notify_all();
			return true;
		}

		unsigned int WaitPushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (Old.Size == MaxLength)
			{
				LOCKFREE_CV(mtxPush, cvPush);
			}
			LOCKFREE_QUEUE_PUSH_BULK;
			cvPop.notify_all();
			return ret;
		}

		unsigned int WaitPopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_CV(mtxPop, cvPop);
			}
			LOCKFREE_QUEUE_POP_BULK;
			cvPush.notify_all();
			return ret;
		}

		void StopWait()
		{
			Wait = false;
			cvPop.notify_all();
			cvPush.notify_all();
		}

		~LockFreeBlockQueue_Array()
		{
			if (Values)
				free(Values);
		}
		LockFreeBlockQueue_Array() {};
		Constructor_Delete(LockFreeBlockQueue_Array);
	};

	template <class T>
	class LockFreeQueue_List
	{
		struct Node
		{
			T value;
			Node* next;
		};

		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		Node* Head;
		Node* Tail;
		std::atomic<Param> Lock;

	public:
		void Init()
		{
			Head = nullptr;
			Tail = nullptr;
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		void Push(Type&& inValue)
		{
			LOCKFREE_MALLOC_ONE;
			NewNode->next = nullptr;
			LOCKFREE_PREPARE;
			LOCKFREE_QUEUE_PUSH_LIST();
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_QUEUE_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		void PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_MALLOC_BULK;
			Ptr->next = nullptr;
			LOCKFREE_PREPARE;
			LOCKFREE_QUEUE_PUSH_BULK_LIST();
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_QUEUE_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		~LockFreeQueue_List()
		{
			Node* pFree = Tail;
			if (pFree)
			{
				Tail = Tail->next;
				free(pFree);
				pFree = Tail;
			}
		}
		LockFreeQueue_List() {};
		Constructor_Delete(LockFreeQueue_List);
	};

	template <class T>
	class LockFreeBlockQueue_List
	{
		struct Node
		{
			T value;
			Node* next;
		};

		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		bool Wait;
		Node* Head;
		Node* Tail;
		LockFreeWaiter waiter;
		std::atomic<Param> Lock;

	public:
		void Init()
		{
			Wait = true;
			Head = nullptr;
			Tail = nullptr;
			Lock.store({ 0, true }, std::memory_order_relaxed);
		}

		template <class Type>
		void Push(Type&& inValue)
		{
			LOCKFREE_MALLOC_ONE;
			NewNode->next = nullptr;
			LOCKFREE_PREPARE;
			LOCKFREE_QUEUE_PUSH_LIST(std::this_thread::yield());
			waiter.notify_one();
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_QUEUE_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		void PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_MALLOC_BULK;
			Ptr->next = nullptr;
			LOCKFREE_PREPARE;
			LOCKFREE_QUEUE_PUSH_BULK_LIST(std::this_thread::yield());
			waiter.notify_all();
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_QUEUE_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		bool WaitPop(T& reValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_WAITER;
			}
			LOCKFREE_QUEUE_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		unsigned int WaitPopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_WAITER;
			}
			LOCKFREE_QUEUE_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		void StopWait()
		{
			Wait = false;
			waiter.notify_all();
		}

		~LockFreeBlockQueue_List()
		{
			Node* pFree = Tail;
			if (pFree)
			{
				Tail = Tail->next;
				free(pFree);
				pFree = Tail;
			}
		}
		LockFreeBlockQueue_List() {};
		Constructor_Delete(LockFreeBlockQueue_List);
	};

	template <class T>
	class LockFreeStack_List
	{
		struct Node
		{
			T value;
			Node* next;
		};

		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		Node* Top;
		std::atomic<Param> Lock;

	public:
		void Init()
		{
			Top = nullptr;
			Lock.store({ 0, 1 }, std::memory_order_relaxed);
		}

		template <class Type>
		void Push(Type&& inValue)
		{
			LOCKFREE_MALLOC_ONE;
			LOCKFREE_PREPARE;
			LOCKFREE_STACK_PUSH_LIST();
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_STACK_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		void PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_MALLOC_BULK;
			LOCKFREE_PREPARE;
			LOCKFREE_STACK_PUSH_BULK_LIST();
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_STACK_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		~LockFreeStack_List()
		{
			Node* pFree = Top;
			if (pFree)
			{
				Top = Top->next;
				free(pFree);
				pFree = Top;
			}
		}
		LockFreeStack_List() {};
		Constructor_Delete(LockFreeStack_List);
	};

	template <class T>
	class LockFreeBlockStack_List
	{
		struct Node
		{
			T value;
			Node* next;
		};

		struct Param
		{
			unsigned int Size;
			unsigned int Free;
		};

		bool Wait;
		Node* Top;
		LockFreeWaiter waiter;
		std::atomic<Param> Lock;

	public:
		void Init()
		{
			Wait = true;
			Top = nullptr;
			Lock.store({ 0, true }, std::memory_order_relaxed);
		}

		template <class Type>
		void Push(Type&& inValue)
		{
			LOCKFREE_MALLOC_ONE;
			LOCKFREE_PREPARE;
			LOCKFREE_STACK_PUSH_LIST(std::this_thread::yield());
			waiter.notify_one();
		}

		bool Pop(T& reValue)
		{
			LOCKFREE_PREPARE;
			if (!Old.Size)
				return false;
			LOCKFREE_STACK_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		void PushBulk(T* inValues, unsigned int size)
		{
			LOCKFREE_MALLOC_BULK;
			LOCKFREE_PREPARE;
			LOCKFREE_STACK_PUSH_BULK_LIST(std::this_thread::yield());
			waiter.notify_all();
		}

		unsigned int PopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET;
			if (!Old.Size)
				return 0;
			LOCKFREE_STACK_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		bool WaitPop(T& reValue)
		{
			LOCKFREE_PREPARE_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_WAITER;
			}
			LOCKFREE_STACK_POP_LIST;
			LOCKFREE_FREE_ONE;
			return true;
		}

		unsigned int WaitPopBulk(T* reValues, unsigned int size)
		{
			LOCKFREE_PREPARE_RET_TIMEOUT;
			if (!Old.Size)
			{
				LOCKFREE_WAITER;
			}
			LOCKFREE_STACK_POP_BULK_LIST;
			LOCKFREE_FREE_LIST;
			return ret;
		}

		void StopWait()
		{
			Wait = false;
			waiter.notify_all();
		}

		~LockFreeBlockStack_List()
		{
			Node* pFree = Top;
			while (pFree)
			{
				Node* temp = pFree;
				pFree = pFree->next;
				free(temp);
			}
		}
		LockFreeBlockStack_List() {};
		Constructor_Delete(LockFreeBlockStack_List);
	};
}
#endif // HSLL_LOCKFREE