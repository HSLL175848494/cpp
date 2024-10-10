#ifndef HSLL_MEMPOOL
#define HSLL_MEMPOOL

#include<thread>
#include<semaphore>

namespace HSLL
{

#ifndef Constructor_Delete(cName)

#define Constructor_Delete(cName)\
	cName(const cName&) = delete;\
	cName& operator=(const cName&) = delete;\
	cName(cName&&) = delete;\
	cName& operator=(cName&&) = delete;\

#endif // !Constructor_Delete(cName)

#define Cheak_Free \
	if constexpr (MulThread){\
	if (pBlock->pRef->fetch_sub(1, std::memory_order_relaxed) == 1)_aligned_free(pBlock->pRef);}\
	else{\
	if (--(*pBlock->pRef) == 1)_aligned_free(pBlock->pRef);}

#define Get_MinIndex(MinSize)\
	unsigned char i = 0;\
	unsigned int Size = 1;\
	while (Size < MinSize)\
	{	Size <<= 1; i++;}\

	template<class Node>
	class LockFreeStack
	{
		std::atomic<Node*> Head{ nullptr };
		std::atomic<unsigned int> StackSize{ 0 };//并不保证正确

	public:

		void Push(Node* Ptr)
		{
			Ptr->next = Head.load(std::memory_order_relaxed);
			while (!Head.compare_exchange_weak(Ptr->next, Ptr,
				std::memory_order_release, std::memory_order_relaxed));
			++StackSize;
		}

		Node* Pop()
		{
			Node* OldNode = Head.load(std::memory_order_acquire);
			while (OldNode && !Head.compare_exchange_weak(OldNode, OldNode->next,
				std::memory_order_acquire, std::memory_order_acquire));
			if (OldNode)
				--StackSize;
			return OldNode;
		}

		unsigned int Size() { return StackSize.load(std::memory_order_relaxed); }

		Node* Top() { return Head.load(std::memory_order_relaxed); }
	};

	template<class Node>
	class SimpleStack
	{
		Node* Head{ nullptr };
		unsigned int StackSize{ 0 };

	public:

		void Push(Node* Ptr)
		{
			Ptr->next = Head;
			Head = Ptr;
			++StackSize;
		}

		Node* Pop()
		{
			Node* OldNode = Head;
			if (OldNode)
			{
				Head = OldNode->next;
				--StackSize;
			}
			return OldNode;
		}

		unsigned int Size() { return StackSize; }

		Node* Top() { return Head; }
	};

	struct MemBlock
	{
		MemBlock* next;
		unsigned int index;
		std::atomic<unsigned int>* pRef;
	};

	struct sMemBlock
	{
		sMemBlock* next;
		unsigned int index;
		unsigned int* pRef;
	};

	template<bool T>
	struct PoolType;

	template<>
	struct PoolType<true>
	{
		using type_stack = LockFreeStack<MemBlock>;
		using type_pRef = std::atomic<unsigned int>*;
		using type_ref = std::atomic<unsigned int>;
		using type_pBlock = MemBlock*;
		using type_block = MemBlock;
	};

	template<>
	struct PoolType<false>
	{
		using type_stack = SimpleStack<sMemBlock>;
		using type_pRef = unsigned int*;
		using type_ref = unsigned int;
		using type_pBlock = sMemBlock*;
		using type_block = sMemBlock;
	};

	template<class T>
	struct AllocInfo
	{
		unsigned int Index;
		T* pAlloc;
	};


#define Pool_Factor0 1
#define Pool_Factor1 1
#define Pool_Factor2 0.5
#define Pool_Factor3 0.15
#define Pool_Factor4 0.05
#define Pool_RangeMapping(Index) Index<5?Pool_Factor0:(Index<12?Pool_Factor1:(Index<20?Pool_Factor2:(Index<27?Pool_Factor3:Pool_Factor4)))

	/*
 Since each block stores a pointer to the next block, a significant
amount of space is wasted when requesting small memory allocations.
The memory pool ensures thread safety. However, in high contention
scenarios, its efficiency will be lower than that of a memory pool
using std::mutex for thread safety.Pools create multiple memory pools
at once to reduce contention in multithreaded scenarios, but this is
less efficient than creating a memory pool for each thread.
	*/
	template<unsigned int MinExpected, unsigned int MaxExpected, bool MulThread, unsigned int BlockNum = 10>
	class MemoryPool
	{
		using Type = PoolType<MulThread>;

		unsigned int BlockAlloc[32];
		unsigned int BestSize[32];
		std::thread AllocMonitor;
		std::binary_semaphore MonitorContinue;
		Type::type_stack MemStack[32];//单次最大申请大小为2^31字节
		Type::type_ref AllocTimes[32]{};

		void* AllocIndex(unsigned char Index, unsigned int Size, bool Ret)//申请对应大小的内存
		{
			unsigned int AllocNum = (BlockAlloc[Index] == 0 ? 1 : BlockAlloc[Index]);

			typename Type::type_pRef pRef = (typename Type::type_pRef)_aligned_malloc
			((sizeof(typename Type::type_block) + Size) * AllocNum + sizeof(typename Type::type_ref), 64);

			if (pRef)
			{
				++AllocTimes[Index];
				*pRef = AllocNum;

				typename Type::type_pBlock pBlock = (typename Type::type_pBlock)((unsigned char*)pRef + sizeof(typename Type::type_ref));
				for (unsigned int i = 0; i < AllocNum - 1; i++)
				{
					pBlock->index = Index;
					pBlock->pRef = pRef;
					MemStack[Index].Push(pBlock);
					pBlock = (typename Type::type_pBlock)((unsigned char*)(pBlock + 1) + Size);
				}

				pBlock->index = Index;
				pBlock->pRef = pRef;

				if (Ret)
					return (void*)(pBlock + 1);
				else
					MemStack[Index].Push(pBlock);
			}

			return nullptr;
		}

		void MemAlloc()//用于预先申请内存部分大小内存(不预先申请大内存)
		{
			Get_MinIndex(MinExpected)
				while (Size <= MaxExpected && Size <= 2048)
				{
					AllocIndex(i, Size, false);
					Size <<= 1; i++;
				}
		}

		template<class T>
		T* GetBlock(unsigned int AllocSize)
		{
			if (!AllocSize || AllocSize > 0x80000000)
				return nullptr;
			Get_MinIndex(AllocSize)
				typename Type::type_pBlock pBlock = MemStack[i].Pop();
			if (pBlock)
				return	(T*)(pBlock + 1);
			else
				return (T*)(i > 1 ? AllocIndex(i - 1, AllocSize, true) : AllocIndex(i, Size, true));
		}

		static void Monitor(MemoryPool* This)//根据最近某大小内存分配次数,动态调整内存块数目
		{
			unsigned int AllocTimesPre[32] = { 0 };
			do {
				unsigned int Times;
				for (unsigned int i = 0; i < 32; i++)
				{
					Times = This->AllocTimes[i];
					if (Times != AllocTimesPre[i])
					{
						This->BestSize[i] = (unsigned int)((This->BestSize[i] + 1) * 1.1);
						AllocTimesPre[i] = Times;
					}
					else
						This->BestSize[i] = (unsigned int)(This->BestSize[i] * 0.9);
				}
			} while (!This->MonitorContinue.try_acquire_for(std::chrono::seconds(1)));
		}

	public:

		MemoryPool() :MonitorContinue(0)
		{
			static_assert(MaxExpected >= MinExpected && MinExpected > 0 && BlockNum >= 5,
				"requre template params : 0 < MinExpected < MaxExpected && BlockNum >= 5");

			for (unsigned int i = 0; i < 32; i++)
			{
				double Factor = Pool_RangeMapping(i);
				BlockAlloc[i] = (unsigned int)(Factor * BlockNum);
			}

			memcpy(BestSize, BlockAlloc, sizeof(BlockAlloc));
			AllocMonitor = std::thread(&MemoryPool::Monitor, this);
			MemAlloc();
		}

		template<class T, bool Construct = true, class... Args>
		T* Alloc(Args&&... args)//申请空间
		{
			T* pResult = GetBlock<T>(sizeof(T));

			if constexpr (Construct)
			{
				if (pResult)
				{
					if constexpr (sizeof...(Args))
						new(pResult) T(std::forward<Args>(args)...);
					else
						new(pResult) T;
				}
			}
			return pResult;
		}


		template<class T>
		T* AllocArray(unsigned int ArraySize, unsigned int SetNull = false)//申请数组空间
		{
			unsigned long long AllocSize = sizeof(T) * ArraySize;
			T* pResult = GetBlock<T>(AllocSize);
			if (pResult && SetNull)
				memset(pResult, 0, AllocSize);
			return pResult;
		}

		template<bool Destruct = true, class T>
		void Free(T* pFree)//释放内存
		{
			if (pFree)
			{
				if constexpr (Destruct)
					pFree->~T();

				typename Type::type_pBlock pBlock = (typename Type::type_pBlock)pFree;
				unsigned char Index = (--pBlock)->index;
				if (MemStack[Index].Size() < BestSize[Index])
					MemStack[Index].Push(pBlock);
				else
				{
					Cheak_Free
				}
			}
		}

		~MemoryPool()//释放内存池(还未free的内存将泄露)
		{
			MonitorContinue.release();
			for (int i = 0; i < 32; i++)//根据引用数目释放栈内存()
			{
				typename Type::type_pBlock pBlock = MemStack[i].Pop();
				while (pBlock)
				{
					Cheak_Free
						pBlock = pBlock->next;
				}
			}
			AllocMonitor.join();//等待线程结束
		}

		Constructor_Delete(MemoryPool)
	};

	template<unsigned int MinExpected, unsigned int MaxExpected, unsigned int CreateNum, unsigned int BlockNum = 10>
	class MemoryPools//
	{
		MemoryPool<MinExpected, MaxExpected, true, BlockNum> Pools[CreateNum]{};

		std::atomic <unsigned int> Target = 0;

	public:

		template<class T, bool Construct = true, class... Args>
		AllocInfo<T> Alloc(Args&&... args)//申请空间
		{
			AllocInfo<T> rData;
			rData.Index = Target.fetch_add(1, std::memory_order_relaxed) % CreateNum;
			rData.pAlloc = Pools[rData.Index].Alloc<T, Construct>(std::forward<Args>(args)...);
			return rData;
		}

		template<class T>
		AllocInfo<T> AllocArray(unsigned int ArraySize, unsigned int SetNull = false)//申请数组空间
		{
			AllocInfo<T> rData;
			rData.Index = Target.fetch_add(1, std::memory_order_relaxed) % CreateNum;
			rData.pAlloc = Pools[rData.Index].AllocArray<T>(ArraySize, SetNull);
			return rData;
		}

		template<bool Destruct = true, class T>
		void Free(T* pFree, unsigned int Index = 0xffffffff)//释放内存
		{
			if (Index < CreateNum)
				Pools[Index].Free<Destruct>(pFree);
			else
				Pools[Target.load(std::memory_order_relaxed) % CreateNum].Free<Destruct>(pFree);
		}

		MemoryPools() {};
		Constructor_Delete(MemoryPools)
	};
}
#endif //HSLL_MEMPOOL