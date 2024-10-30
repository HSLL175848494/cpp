#ifndef HSLL_MEMPOOL
#define HSLL_MEMPOOL

#include "Stack.h"

namespace HSLL
{
    struct BlockHeadSgl
    {
        unsigned int ref;
        unsigned int index;
    };

    struct BlockHeadMul
    {
        std::atomic<unsigned int> ref;
        unsigned int index;
    };

    struct MemBlockSgl
    {
        MemBlockSgl* next;
        BlockHeadSgl* head;
    };

    struct MemBlockMul
    {
        MemBlockMul* next;
        BlockHeadMul* head;
    };

    template <bool T>
    struct StackType;

    template <>
    struct StackType<true>
    {
        using type = LockFreeStack<MemBlockMul>;
        using type_head = BlockHeadMul;
        using type_block = MemBlockMul;
    };

    template <>
    struct StackType<false>
    {
        using type = SimpleStack<MemBlockSgl>;
        using type_head = BlockHeadSgl;
        using type_block = MemBlockSgl;
    };

    struct ThreadCache
    {
        unsigned int maxSize;
        unsigned int nowSize = 0;
        void* pBlock = nullptr;
    };

    thread_local ThreadCache cache[32]{ {5}, {5}, {5}, {5}, {4}, {4}, {4}, {3}, {3}, {2}, {1}, {1}, {1}, {1}, {1}, 
        {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1}, {1} };

#define BYTE_MOVE(p, size) ((unsigned char *)p + size)

    template <bool MulThread>
    class MemPool
    {
        using MemStack = typename StackType<MulThread>::type;
        using MemBlock = typename StackType<MulThread>::type_block;
        using BlockHead = typename StackType<MulThread>::type_head;

        MemStack stacks[32];                    // 2^0-2^31次方大小内存块
        static unsigned int allocMap[32];       // 每次应该申请多少内存块
        static unsigned int indexAllocSize[32]; // 对应内存块的大小

    public:
        void* AllocIndex(unsigned char index)
        {
            unsigned int size = indexAllocSize[index];
            unsigned int num = allocMap[index];
            BlockHead* pHead = (BlockHead*)malloc(sizeof(BlockHead) + size * num);
            pHead->ref = num;
            pHead->index = index;

            MemBlock* pAlloc = (MemBlock*)(pHead + 1);
            MemBlock* ptr = pAlloc;
            ptr->head = pHead;
            ptr->next = (MemBlock*)BYTE_MOVE(ptr, size);
            for (unsigned int i = 0; i < num - 1; i++)
            {
                ptr = ptr->next;
                ptr->head = pHead;
                ptr->next = (MemBlock*)BYTE_MOVE(ptr, size);
            }

            if (num != 1)
            {
                MemBlock* pPush = pAlloc;
                if constexpr (MulThread)
                {
                    unsigned int pushNum = cache[index].maxSize - 1;
                    if (pushNum)
                    {
                        pPush = (MemBlock*)BYTE_MOVE(pPush, pushNum * size);
                        pPush->next = nullptr;
                        cache[index].pBlock = (MemBlock*)BYTE_MOVE(pAlloc, size); // 部分放入线程缓冲
                        cache[index].nowSize = pushNum;
                    }
                }
                stacks[index].Push_List((MemBlock*)BYTE_MOVE(pPush, size), ptr); // 返回全局栈
            }
            return pAlloc + 1; // 返回第一个块的内存
        }

        template <class T, bool Construct = true, class... Args>
        T* Alloc(Args &&...args) // 申请空间
        {
            unsigned int size = sizeof(T) + sizeof(MemBlock);
            unsigned int index = 0;
            while (indexAllocSize[index] < size)
                index++;

            T* pAlloc;
            MemBlock* pBlock;

            if constexpr (MulThread)
            {
                if (cache[index].nowSize)
                {
                    pBlock = (MemBlock*)cache[index].pBlock;
                    cache[index].pBlock = pBlock->next;
                    pAlloc = (T*)(pBlock + 1);
                    cache[index].nowSize--;
                    goto sign;
                }
            }

            pBlock = stacks[index].Pop();
            if (pBlock)
                pAlloc = (T*)(pBlock + 1);
            else
                pAlloc = (T*)AllocIndex(index);

        sign:

            if constexpr (Construct)
            {
                if (pAlloc)
                {
                    if constexpr (sizeof...(Args))
                        new (pAlloc) T(std::forward<Args>(args)...);
                    else
                        new (pAlloc) T;
                }
            }
            return pAlloc;
        }

        template <class T>
        T* AllocArray(unsigned int ArraySize, unsigned int SetNull = false) // 申请数组空间
        {
            unsigned int size = sizeof(T) * ArraySize + sizeof(MemBlock);
            unsigned int index = 0;
            while (indexAllocSize[index] < size)
                index++;

            T* pAlloc;
            MemBlock* pBlock;

            if constexpr (MulThread)
            {
                if (cache[index].nowSize)
                {
                    pBlock = (MemBlock*)cache[index].pBlock;
                    cache[index].pBlock = pBlock->next;
                    cache[index].nowSize--;
                    pAlloc = (T*)(pBlock + 1);
                    goto sign;
                }
            }

            pBlock = stacks[index].Pop();

            if (pBlock)
                pAlloc = (T*)(pBlock + 1);
            else
                pAlloc = (T*)AllocIndex(index);

        sign:

            if (pAlloc && SetNull)
                memset(pAlloc, 0, size - sizeof(MemBlock));
            return pAlloc;
        }

        template <bool Destruct = true, class T>
        void Free(T* pFree) // 释放内存
        {
            if constexpr (Destruct)
                pFree->~T();

            MemBlock* pBlock = (MemBlock*)pFree - 1;
            unsigned int index = pBlock->head->index;
            if constexpr (MulThread)
            {
                pBlock->next = (MemBlock*)cache[index].pBlock;
                if (cache[index].nowSize == cache[index].maxSize)
                {
                    stacks[index].Push_List(pBlock);
                    cache[index].pBlock = nullptr;
                    cache[index].nowSize = 0;
                }
                else
                {
                    cache[index].nowSize++;
                    cache[index].pBlock = pBlock;
                }
                return;
            }
            stacks[index].Push(pBlock);
        }

        template <bool Destruct = true, class T>
        void FreeOnly(T* pFree) // 释放内存
        {
            if constexpr (Destruct)
                pFree->~T();

            MemBlock* pBlock = (MemBlock*)pFree - 1;
            if constexpr (MulThread)
            {
                if (pBlock->head->ref.fetch_sub(1, std::memory_order_relaxed) == 1)
                    free(pBlock->head);
            }
            else
            {
                if ((pBlock->head->ref--) == 1)
                    free(pBlock->head);
            }
        }

        void FreeCache()
        {
            for (unsigned int i = 0; i < 32; i++)
            {
                if (cache[i].pBlock)
                    stacks[i].Push_List((MemBlock*)cache[i].pBlock);
            }
        }

        ~MemPool()
        {
            MemBlock* pTemp;
            MemBlock* pFree;
            for (unsigned int i = 0; i < 32; i++)
            {
                pFree = stacks[i].Pop();
                while (pFree)
                {
                    pTemp = pFree->next;
                    if (pFree->head->ref == 1)
                        free(pFree->head);
                    else
                        pFree->head->ref--;
                    pFree = pTemp;
                }
            }
        }
    };

    template <bool MulThread>
    unsigned int MemPool<MulThread>::indexAllocSize[]{
        1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608, 16777216,
        33554432, 67108864, 134217728, 268435456, 536870912, 1073741824, 2147483648 };

    template <bool MulThread>
    unsigned int MemPool<MulThread>::allocMap[]{ 50, 50, 50, 50, 25, 25, 25, 15,
                                                15, 10, 5, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

}

#endif // HSLL_MEMPOOL