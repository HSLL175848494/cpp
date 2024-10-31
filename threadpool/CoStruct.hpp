#ifndef HSLL_COSTRUCT
#define HSLL_COSTRUCT

#include <mutex>
#include <condition_variable>

namespace HSLL
{

    template <class T1, class T2>
    struct SameType
    {
        static const bool value = false;
    };

    template <class T1>
    struct SameType<T1, T1>
    {
        static const bool value = true;
    };

    template <class T1, class T2>
    static const bool is_same_type_v = SameType<T1, T2>::value;

    template <class T>
    class Stack
    {
        T* values;
        unsigned int nowSize;
        unsigned int maxSize;

    public:

        bool full()
        {
            return nowSize == maxSize;
        }

        bool empty()
        {
            return !nowSize;
        }

        unsigned int Size()
        {
            return nowSize;
        }

        unsigned int MaxSize() const
        {
            return maxSize;
        }

        bool Push(const T& value)
        {
            if (nowSize == maxSize)
                return false;
            values[nowSize++] = value;
            return true;
        }

        bool Push(T&& value)
        {
            if (nowSize == maxSize)
                return false;
            values[nowSize++] = std::move(value);
            return true;
        }

        bool Pop()
        {
            if (nowSize == 0)
                return false;
            --nowSize;
            return true;
        }

        T& First()
        {
            if (nowSize == 0)
                throw std::out_of_range("stack is empty");
            return values[nowSize - 1];
        }

        void Init(unsigned int maxSize)
        {
            this->maxSize = maxSize;
            values = new T[maxSize];
        }

        Stack() : values(nullptr), nowSize(0) {};

        ~Stack()
        {
            if (values)
                delete[] values;
        }
    };

    template <class T>
    class Queue
    {
        T* values;
        unsigned int front;
        unsigned int back;
        unsigned int nowSize;
        unsigned int maxSize;

    public:

        bool full()
        {
            return nowSize == maxSize;
        }

        bool empty()
        {
            return !nowSize;
        }

        unsigned int Size()
        {
            return nowSize;
        }

        unsigned int MaxSize() const
        {
            return maxSize;
        }

        bool Push(const T& value)
        {
            if (nowSize == maxSize)
                return false;
            values[back] = value;
            back = (back + 1) % maxSize;
            nowSize++;
            return true;
        }

        bool Push(T&& value)
        {
            if (nowSize == maxSize)
                return false;
            values[back] = std::move(value);
            back = (back + 1) % maxSize;
            nowSize++;
            return true;
        }

        bool Pop()
        {
            if (nowSize == 0)
                return false;
            front = (front + 1) % maxSize;
            nowSize--;
            return true;
        }

        T& First()
        {
            if (nowSize == 0)
                throw std::out_of_range("queue is empty");
            return values[front];
        }

        void Init(unsigned int maxSize)
        {
            this->maxSize = maxSize;
            values = new T[maxSize];
        }

        Queue() : values(nullptr), front(0), back(0), nowSize(0) {};

        ~Queue()
        {
            if (values)
                delete[] values;
        }
    };

    template <template <typename> class V, typename T>
    class CoStruct
    {
        V<T*> v;
        std::mutex mtx;
        std::condition_variable cv;
        bool flag;

    public:
        CoStruct() : flag(true)
        {
            static_assert(is_same_type_v<V<T>, Queue<T>> || is_same_type_v<V<T>, Stack<T>>,
                "CoStruct typename V must be Stack or Queue in namespace HSLL");
        }

        void Init(unsigned int maxSize)
        {
            v.Init(maxSize);
        }

        unsigned int MaxSize()
        {
            return v.MaxSize();
        }

        bool Push(T* value)
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                if (!v.Push(value))
                    return false;
            }
            cv.notify_one();
            return true;
        }

        T* Pop()
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (v.empty())
                return nullptr;
            T* value = v.First();
            v.Pop();
            return value;
        }

        T* WaitPop()
        {
            std::unique_lock<std::mutex> lock(mtx);
            cv.wait(lock, [this] { return !flag || !v.empty(); });
            if (!flag)
                return nullptr;
            T* value = v.First();
            v.Pop();
            return value;
        }

        void StopWait()
        {
            {
                std::lock_guard<std::mutex> lock(mtx);
                flag = false;
            }
            cv.notify_all();
        }
    };
}

#endif // HSLL_COSTRUCT