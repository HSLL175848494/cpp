#ifndef HSLL_COROUTINE
#define HSLL_COROUTINE

#include <coroutine>
#include <optional>

namespace HSLL
{
    template <bool Suspend>
    struct Suspend_Type;

    template <>
    struct Suspend_Type<true>
    {
        using value = std::suspend_always;
    };

    template <>
    struct Suspend_Type<false>
    {
        using value = std::suspend_never;
    };

    template <bool StartSuspend, typename... Args>
    class Generator;

    template <bool StartSuspend, class T>
    class Generator<StartSuspend, T>//拥有返回值
    {
    public:
        struct promise_type
        {
            std::optional<T> optional;

            auto get_return_object() { return Generator{std::coroutine_handle<promise_type>::from_promise(*this)}; }

            void return_value(T value) { optional = value; }
            void unhandled_exception() { std::terminate(); }

            Suspend_Type<StartSuspend>::value initial_suspend() { return {}; }
            std::suspend_always final_suspend() noexcept { return {}; }
            std::suspend_always yield_value(T value)
            {
                optional = value;
                return {};
            }
        };

    private:
        std::coroutine_handle<promise_type> handle;

    public:
        Generator(std::coroutine_handle<promise_type> handle) : handle(handle) {}

        ~Generator()
        {
            if (handle)
                handle.destroy();
        }

        bool hasDone()
        {
            if (handle)
                return handle.done();
            return false;
        }

        bool Resume()
        {
            if (handle && !handle.done())
            {
                handle.resume();
                return true;
            }
            else
                return false;
        }

        std::optional<T> next()
        {
            if (!handle.done())
            {
                handle.resume();
                return handle.promise().optional;
            }
            else
                return std::nullopt;
        }

        std::optional<T> Value()
        {
            if (handle)
                return handle.promise().optional;
            else
                return std::nullopt;
        }

        Generator &operator=(Generator &&other)
        {
            if (this != &other)
            {
                if (handle)
                    handle.destroy();
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }

        Generator(const Generator &) = delete;
        Generator &operator=(const Generator &) = delete;
        Generator(Generator &&other) = delete;
    };

    template <bool StartSuspend>
    class Generator<StartSuspend>//无返回值
    {
    public:
        struct promise_type
        {
            Generator get_return_object() { return Generator{std::coroutine_handle<promise_type>::from_promise(*this)}; }

            void return_void() {}

            void unhandled_exception() { std::terminate(); }

            Suspend_Type<StartSuspend>::value initial_suspend() { return {}; }

            std::suspend_always final_suspend() noexcept { return {}; }
        };

    private:
        std::coroutine_handle<promise_type> handle;

    public:
        Generator(std::coroutine_handle<promise_type> handle) : handle(handle) {}

        ~Generator()
        {
            if (handle)
                handle.destroy();
        }

        bool hasDone()
        {
            if (handle)
                return handle.done();
            return false;
        }

        bool Resume()
        {
            if (handle && !handle.done())
            {
                handle.resume();
                return true;
            }
            else
                return false;
        }

        Generator &operator=(Generator &&other)
        {
            if (this != &other)
            {
                if (handle)
                    handle.destroy();
                handle = other.handle;
                other.handle = nullptr;
            }
            return *this;
        }
        Generator(const Generator &) = delete;
        Generator &operator=(const Generator &) = delete;
        Generator(Generator &&other) = delete;
    };
}

#endif