线程安全的可申请任意数据类型无的锁内存池，并进行构造与析构
使用时需要包含：Mempool.hpp(依赖于Stack.h)
方法均包含于命名空间HSLL；

构:

template <bool MulThread>
class MemPool

(MulThread:是否多线程)

方法:

template <class T, bool Construct = true, class... Args>
T* Alloc(Args &&...args)

申请空间，默认进行构造函数的调用,可变参数
      
template <class T>
T* AllocArray(unsigned int ArraySize, unsigned int SetNull = false)

申请数组，默认不初始化为0

template <bool Destruct = true, class T>
void Free(T* pFree)

释放指针，默认调用析构。如果析构是已删除函数可置为false

template <bool Destruct = true, class T>
void FreeOnly(T* pFree) 

直接释放指针，减少引用计数，不会将其从新利用(对于那些一次性分配对于大小内存，但后续不使用时内存将闲置。此时可以直接选择释放)

void FreeCache()

释放线程缓存，将其中的内存块返回到通用内存块。当类的MulThread模板为true时，如果线程有通过pool申请空间，请在线程退出时调用该函数

释放:

多线程模式下注意每个线程结束时线程内对FreeCache()的调用
当pool销毁时会自动清除所有已经申请的内存。注意在此之前调用free归还所有申请的内存
