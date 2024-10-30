线程安全的无锁内存池
使用时需要包含：Mempool.hpp(依赖于Stack.h)
方法均包含于命名空间HSLL；

构造:

template <bool MulThread>
class MemPool


方法：

template <class T, bool Construct = true, class... Args>
T* Alloc(Args &&...args)
      
template <class T>
T* AllocArray(unsigned int ArraySize, unsigned int SetNull = false)

template <bool Destruct = true, class T>
void Free(T* pFree)

template <bool Destruct = true, class T>
void FreeOnly(T* pFree) 

void FreeCache()

