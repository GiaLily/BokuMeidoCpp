/*  bokumeidocpp库的线程相关工具  */
#pragma once
#ifndef THREAD_HPP_BOKUMEIDOCPP
#define THREAD_HPP_BOKUMEIDOCPP

#include <atomic>
#include <condition_variable>
#include <exception>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

#include "base.hpp"
#include "log.hpp"
#include "type.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace thrd
{
    /*  简易自旋锁，适用于临界区操作非常少的情况，线程安全
        - 标准库mutex的实现通常自带自旋锁优化，因此在大多数情况下并不推荐使用SpinLock
        - 单核环境下，自旋锁的效率通常较低
        - 加锁与解锁必须成对，否则会导致死锁或未定义行为
        - 析构时如果有未解锁的情况，会导致未定义行为  */
    class SpinLock final
    {
    private:
        class Unlock;

    public:
        SpinLock() = default;

        /*  对局部区域加锁并自动解锁。调用时加锁，返回对象析构时解锁
            - 用法：auto guard = spin_lock.lockScope();
            @return 一个ScopeGuard对象，只能用auto推导  */

        // 手动加锁
        void lock();
        // 尝试加锁，成功返回true
        bool tryLock();
        // 手动解锁
        void unlock();

        /*  对局部区域加锁并自动解锁。调用时加锁，返回对象析构时解锁
            - 用法：auto guard = spin_lock.lockScope();
            @return 一个ScopeGuard对象，只能用auto推导  */
        base::ScopeGuard<Unlock> lockScope();

        SpinLock(const SpinLock& tmp_lock) = delete;
        SpinLock& operator=(const SpinLock& tmp_lock) = delete;

    private:
        std::atomic_flag lock_flag_ = ATOMIC_FLAG_INIT;
    };

    /*  无优先级的读写锁，线程安全
        - 加锁与解锁必须成对，否则会导致死锁或未定义行为
        - 析构时如果有未解锁的情况，会导致未定义行为  */
    class ReadWriteLock final
    {
    private:
        class UnlockR;
        class UnlockW;

    public:
        ReadWriteLock() = default;

        /*  对局部区域加读锁并自动解锁。调用时加锁，返回对象析构时解锁
            - 用法：auto guard = rw_lock.lockReadScope();
            @return 一个ScopeGuard对象，只能用auto推导  */
        // 手动加读锁
        void lockRead();
        // 尝试加读锁，成功返回true
        bool tryLockRead();
        // 手动解读锁
        void unlockRead();

        // 手动加写锁
        void lockWrite();
        // 尝试加写锁，成功返回true
        bool tryLockWrite();
        // 手动解写锁
        void unlockWrite();

        /*  对局部区域加读锁并自动解锁。调用时加锁，返回对象析构时解锁
            - 用法：auto guard = rw_lock.lockReadScope();
            @return 一个ScopeGuard对象，只能用auto推导  */
        base::ScopeGuard<UnlockR> lockReadScope();

        /*  对局部区域加写锁并自动解锁。调用时加锁，返回对象析构时解锁
            - 用法：auto guard = rw_lock.lockWriteScope();
            @return 一个ScopeGuard对象，只能用auto推导  */
        base::ScopeGuard<UnlockW> lockWriteScope();

        // 禁止拷贝和移动
        ReadWriteLock(const ReadWriteLock& tmp_lock) = delete;
        ReadWriteLock& operator=(const ReadWriteLock& tmp_lock) = delete;

    private:
        std::mutex mtx_;
        std::condition_variable cv_;
        size_t num_readers_ = 0;
        size_t num_waiting_writers_ = 0;
        bool is_writing_ = false;
    };

    class ThreadPool;

    /*  任务的future
        - Ret不可为引用类型
        - Ret必须为void或可移动构造的类型（确保能被packaged_task存入共享状态）
        - 由于编译器的设定，对于非引用的基本类型，Ret不保留顶层cv修饰（如对于返回const int的函数应使用TaskFuture<int>） */
    template <class Ret>
    class TaskFuture final
    {
    public:
        // 构造一个无效的TaskFuture对象（Ret必须可移动构造以确保能被packaged_task存入共享状态）
        template <class RetU = Ret, typename std::enable_if<std::is_same<RetU, Ret>::value && !std::is_reference<RetU>::value && (std::is_void<RetU>::value || std::is_move_constructible<RetU>::value), int>::type = 0>
        TaskFuture() {};

        // 判断任务是否为有效状态；valid、finished、wait、get之间线程安全
        bool valid() const;
        // 判断任务是否结束，如果任务为无效状态会返回true；valid、finished、wait、get之间线程安全
        bool finished() const;
        // 等待任务结束，如果任务为无效状态会立即返回；valid、finished、wait、get之间线程安全
        void wait() const;

        /*  等待并获取任务结果的指针，避免抛出异常
            - valid、finished、wait、get之间线程安全
            - 任务无效时返回nullptr
            - Ret为void时也返回nullptr
            - 线程池析构时任务未执行或任务执行失败（抛出异常）时，返回nullptr
            - 返回的指针在TaskFuture对象更新或析构时失效
            @return 任务返回值的指针  */
        const Ret* get() const;
        // 导出标准库future
        std::shared_future<Ret> toFuture() const;

        // 支持移动禁止拷贝
        TaskFuture(TaskFuture<Ret>&& task_future) noexcept;
        TaskFuture& operator=(TaskFuture<Ret>&& task_future) noexcept;

    private:
        template <class RetU, typename std::enable_if<std::is_void<RetU>::value, int>::type = 0>
        const RetU* getPtrDispatch() const;
        template <class RetU, typename std::enable_if<!std::is_void<RetU>::value, int>::type = 0>
        const RetU* getPtrDispatch() const;

        std::shared_future<Ret> future_state_;
        friend class thrd::ThreadPool;
    };

    // 简易线程池，线程安全
    class ThreadPool final
    {
    public:
        /*  构造ThreadPool对象
            @param pool_size: 线程池线程数量，不小于1  
            @param thread_init_func: 每个后台线程启动时在其自身线程上下文中同步执行的初始化操作。
                - 该函数在对应后台线程内被调用（不是异步在其他线程执行），可安全获取和操作当前线程的相关信息（如 thread id 等）。
                - 参数 thread_idx 表示该后台线程在线程池中的索引，从0开始  */
        ThreadPool(uint32_t pool_size, std::function<void(uint32_t thread_idx)> thread_init_func = {});

        /*  添加一个任务到线程池中并异步执行
            @param func: 无参可调用对象（函数 / lambda / 仿函数等）。
                - func必须可以无参调用
                - func的返回类型Ret必须符合TaskFuture<Ret>的构造限制
                - func必须支持拷贝和移动构造
            @return 任务结果的future状态，用于查询任务状态、等待任务结束以及获取任务返回值，注意ThreadPool对象析构后未执行的任务TaskFuture失效  */
        template <class Fn, class Ret = decltype(std::declval<Fn&>()()),
                  typename std::enable_if<std::is_move_constructible<Fn>::value && std::is_copy_constructible<Fn>::value, int>::type = 0,
                  typename std::enable_if<std::is_same<Ret, decltype(std::declval<Fn&>()())>::value && std::is_constructible<TaskFuture<Ret>>::value, int>::type = 0>
        TaskFuture<Ret> addTask(Fn&& func);

        // 是否线程池当前已占满
        bool full();

        // 禁止拷贝和移动
        ThreadPool(const ThreadPool& thd_pool) = delete;
        ThreadPool& operator=(const ThreadPool& thd_pool) = delete;
        // 执行完当前正在执行的任务，放弃队列里剩余的任务，释放资源
        ~ThreadPool();

    private:
        void worker(std::function<void()> init_func);

        uint32_t pool_size_;
        std::queue<std::function<void()>> task_queue_;
        std::vector<std::thread> work_thrds_;
        std::atomic<uint32_t> working_task_num_{0};

        std::mutex task_mtx_;
        std::condition_variable cond_var_;
        std::atomic<bool> need_abort_;
    };

}    // namespace thrd





/*--------------------------------------------内部实现--------------------------------------------*/

namespace thrd
{
    /* --------------------------------- SpinLock 实现 -------------------------------- */

    class SpinLock::Unlock
    {
    public:
        void operator()()
        {
            resource_->unlock();
        }

        Unlock(Unlock&& tmp) noexcept :
            resource_(tmp.resource_)
        {}

        Unlock& operator=(Unlock&& tmp) = delete;

    private:
        Unlock(SpinLock* resource)
        {
            resource_ = resource;
        }

        SpinLock* resource_ = nullptr;
        friend SpinLock;
    };

    inline void SpinLock::lock()
    {
        int spin_count = 0;
        while (lock_flag_.test_and_set(std::memory_order_acquire))
        {
            ++spin_count;
            if (spin_count >= 1000)
            {
                std::this_thread::yield();    // 自旋次数过多，让出CPU
                spin_count = 0;
            }
        }
    }

    inline void SpinLock::unlock()
    {
        lock_flag_.clear(std::memory_order_release);
    }

    inline bool SpinLock::tryLock()
    {
        return !lock_flag_.test_and_set(std::memory_order_acquire);
    }

    inline base::ScopeGuard<SpinLock::Unlock> SpinLock::lockScope()
    {
        this->lock();
        return base::makeScopeGuard(Unlock(this));
    }


    /* --------------------------------- ReadWriteLock 实现 -------------------------------- */

    class ReadWriteLock::UnlockR
    {
    public:
        void operator()()
        {
            resource_->unlockRead();
        }

        UnlockR(UnlockR&& tmp) noexcept
        {
            resource_ = tmp.resource_;
        }
        UnlockR& operator=(UnlockR&& tmp) = delete;

    private:
        UnlockR(ReadWriteLock* resource) :
            resource_(resource)
        {}

        ReadWriteLock* resource_ = nullptr;
        friend ReadWriteLock;
    };

    class ReadWriteLock::UnlockW final
    {
    public:
        void operator()()
        {
            resource_->unlockWrite();
        }

        // 只允许移动构造，不允许移动赋值和拷贝
        UnlockW(UnlockW&& tmp) noexcept
        {
            resource_ = tmp.resource_;
        }

    private:
        UnlockW(ReadWriteLock* resource) :
            resource_(resource)
        {}

        ReadWriteLock* resource_ = nullptr;
        friend ReadWriteLock;
    };

    inline void ReadWriteLock::lockRead()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        while (is_writing_ || num_waiting_writers_ != 0)
            cv_.wait(lk);
        num_readers_++;
    }

    inline void ReadWriteLock::unlockRead()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            num_readers_--;
            if (num_readers_ != 0)
                return;
        }
        cv_.notify_one();
    }

    inline bool ReadWriteLock::tryLockRead()
    {
        if (!mtx_.try_lock())
            return false;
        if (is_writing_ || num_waiting_writers_ != 0)
        {
            mtx_.unlock();
            return false;
        }
        num_readers_++;
        mtx_.unlock();
        return true;
    }

    inline base::ScopeGuard<ReadWriteLock::UnlockR> ReadWriteLock::lockReadScope()
    {
        this->lockRead();
        return base::makeScopeGuard(UnlockR(this));
    }

    inline void ReadWriteLock::lockWrite()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        num_waiting_writers_++;
        while (is_writing_ || num_readers_ != 0)
            cv_.wait(lk);
        num_waiting_writers_--;
        is_writing_ = true;
    }

    inline void ReadWriteLock::unlockWrite()
    {
        {
            std::lock_guard<std::mutex> lk(mtx_);
            is_writing_ = false;
        }
        cv_.notify_all();
    }

    inline bool ReadWriteLock::tryLockWrite()
    {
        if (!mtx_.try_lock())
            return false;
        if (is_writing_ || num_readers_ != 0)
        {
            mtx_.unlock();
            return false;
        }
        is_writing_ = true;
        mtx_.unlock();
        return true;
    }

    inline base::ScopeGuard<ReadWriteLock::UnlockW> ReadWriteLock::lockWriteScope()
    {
        this->lockWrite();
        return base::makeScopeGuard(UnlockW(this));
    }

    /* --------------------------------- TaskFuture 实现 -------------------------------- */

    template <class Ret>
    inline TaskFuture<Ret>::TaskFuture(TaskFuture<Ret>&& tmp_state) noexcept
    {
        future_state_ = std::move(tmp_state.future_state_);
    }

    template <class Ret>
    inline TaskFuture<Ret>& TaskFuture<Ret>::operator=(TaskFuture<Ret>&& tmp_state) noexcept
    {
        if (this != &tmp_state)
            future_state_ = std::move(tmp_state.future_state_);
        return *this;
    }

    template <class Ret>
    inline bool TaskFuture<Ret>::valid() const
    {
        return future_state_.valid();
    }

    template <class Ret>
    inline bool TaskFuture<Ret>::finished() const
    {
        if (future_state_.valid())
            return future_state_.wait_for(std::chrono::nanoseconds(0)) == std::future_status::ready;
        // MEIDO_WARN("Task is invalid, so the function returns value:true");
        return true;
    }

    template <class Ret>
    inline void TaskFuture<Ret>::wait() const
    {
        if (future_state_.valid())
            future_state_.wait();
        // else
            // MEIDO_WARN("Task is invalid, so the function returns directly");
    }

    template <class Ret>
    inline const Ret* TaskFuture<Ret>::get() const
    {
        return this->getPtrDispatch<Ret>();
    }

    template <class Ret>
    inline std::shared_future<Ret> TaskFuture<Ret>::toFuture() const
    {
        return future_state_;
    }

    template <class Ret>
    template <class RetU, typename std::enable_if<std::is_void<RetU>::value, int>::type>
    inline const RetU* TaskFuture<Ret>::getPtrDispatch() const
    {
        return nullptr;
    }

    template <class Ret>
    template <class RetU, typename std::enable_if<!std::is_void<RetU>::value, int>::type>
    inline const RetU* TaskFuture<Ret>::getPtrDispatch() const
    {
        if (future_state_.valid())
        {
            try
            {
                return &future_state_.get();
            }
            catch (const std::exception& e)
            {
                MEIDO_WARN("TaskFuture::get failed: {}", e.what());
                return nullptr;
            }
            catch (...)
            {
                MEIDO_WARN("TaskFuture::get failed: unknown exception");
                return nullptr;
            }
        }
        return nullptr;
    }

    inline ThreadPool::ThreadPool(uint32_t pool_size, std::function<void(uint32_t thread_idx)> thread_init_func)
    {
        if (pool_size <= 0)
        {
            MEIDO_WARN("Invalid param value pool_size:{}, which will be set to 1", pool_size);
            pool_size = 1;
        }
        pool_size_ = pool_size;
        std::queue<std::function<void()>> empty_queue;
        task_queue_.swap(empty_queue);
        need_abort_ = false;

        work_thrds_.resize(pool_size);
        for (uint32_t i = 0; i < pool_size; ++i)
        {
            work_thrds_[i] = std::thread(&ThreadPool::worker, this, [i, thread_init_func] { if (thread_init_func) thread_init_func(i); });
        }
    }

    inline ThreadPool::~ThreadPool()
    {
        {
            std::unique_lock<std::mutex> lk(task_mtx_);
            need_abort_ = true;
            std::queue<std::function<void()>> tmp;
            task_queue_.swap(tmp);
        }
        cond_var_.notify_all();
        for (auto& thd : work_thrds_)
        {
            if (thd.joinable())
                thd.join();
        }
    }

    template <class Fn, class Ret, typename std::enable_if<std::is_move_constructible<Fn>::value && std::is_copy_constructible<Fn>::value, int>::type,
              typename std::enable_if<std::is_same<Ret, decltype(std::declval<Fn&>()())>::value && std::is_constructible<TaskFuture<Ret>>::value, int>::type>
    inline TaskFuture<Ret> ThreadPool::addTask(Fn&& func)
    {
        auto task = std::make_shared<std::packaged_task<Ret()>>(std::forward<Fn>(func));
        TaskFuture<Ret> state;
        state.future_state_ = task->get_future();
        {
            std::lock_guard<std::mutex> lk(task_mtx_);
            task_queue_.emplace([task]() { (*task)(); });
        }
        cond_var_.notify_one();
        return state;
    }

    inline bool ThreadPool::full()
    {
        std::lock_guard<std::mutex> lk(task_mtx_);
        return (working_task_num_.load(std::memory_order_acquire) + task_queue_.size()) >= pool_size_;
    }

    inline void ThreadPool::worker(std::function<void()> init_func)
    {
        init_func();

        std::function<void()> task;
        while (!need_abort_)
        {
            {
                std::unique_lock<std::mutex> lk(task_mtx_);
                while (!need_abort_ && task_queue_.empty())
                {
                    cond_var_.wait(lk);
                }
                if (need_abort_)
                    break;
                task = std::move(task_queue_.front());
                task_queue_.pop();
            }
            working_task_num_.fetch_add(1, std::memory_order_release);
            task();
            working_task_num_.fetch_add(-1, std::memory_order_release);
        }
    }
}    // namespace thrd
}    // namespace meido

#endif    // !THREAD_HPP_BOKUMEIDOCPP