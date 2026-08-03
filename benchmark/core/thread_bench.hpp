/*  thread 模块性能测试
    测试内容：
    - SpinLock 加锁/解锁开销（低竞争）
    - ReadWriteLock 读锁/写锁开销
    - ThreadPool 任务提交与调度开销
*/
#pragma once
#ifndef BENCH_THREAD_HPP
#define BENCH_THREAD_HPP

#include "bokumeido/core.hpp"
#include "bench_common.hpp"
#include <atomic>
#include <thread>
#include <vector>

using namespace meido;

namespace _meidothrdbench
{

inline void check()
{
    BENCH_MODULE("thread 模块");

    BENCH_CALIBRATE();
    BENCH_SEP();

    // ---- 1. SpinLock 单线程加锁/解锁 ----
    {
        thrd::SpinLock lock;
        BENCH_N("SpinLock lock/unlock (单线程, 无竞争)", {
            lock.lock();
            lock.unlock();
        });
    }
    BENCH_SEP();

    // ---- 2. SpinLock tryLock ----
    {
        thrd::SpinLock lock;
        BENCH_N("SpinLock tryLock (单线程, 无竞争)", {
            volatile bool ok = lock.tryLock();
            if (ok) lock.unlock();
            (void)ok;
        });
    }
    BENCH_SEP();

    // ---- 3. SpinLock lockScope RAII ----
    {
        thrd::SpinLock lock;
        BENCH_N("SpinLock lockScope (RAII, 单线程)", {
            auto guard = lock.lockScope();
            (void)guard;
        });
    }
    BENCH_SEP();

    // ---- 4. ReadWriteLock 读锁 ----
    {
        thrd::ReadWriteLock rw_lock;
        BENCH_N("ReadWriteLock lockRead/unlockRead (单线程)", {
            rw_lock.lockRead();
            rw_lock.unlockRead();
        });
    }
    BENCH_SEP();

    // ---- 5. ReadWriteLock 写锁 ----
    {
        thrd::ReadWriteLock rw_lock;
        BENCH_N("ReadWriteLock lockWrite/unlockWrite (单线程)", {
            rw_lock.lockWrite();
            rw_lock.unlockWrite();
        });
    }
    BENCH_SEP();

    // ---- 6. ReadWriteLock 读锁 RAII ----
    {
        thrd::ReadWriteLock rw_lock;
        BENCH_N("ReadWriteLock lockReadScope (RAII)", {
            auto guard = rw_lock.lockReadScope();
            (void)guard;
        });
    }
    BENCH_SEP();

    // ---- 7. ThreadPool 批量提交并等待全部完成 ----
    // 设计意图：批量提交 N 个任务，逐个等待完成（端到端：提交+调度+执行）。
    // 与第 8 项单任务往返互补；语义明确、结果可复现（不依赖 sleep）。
    {
        thrd::ThreadPool pool(4);
        const int64_t N = 10000;
        BENCH_N("ThreadPool 批量提交10000任务+等待完成 (4线程)", {
            std::vector<thrd::TaskFuture<void>> futs;
            futs.reserve(static_cast<size_t>(N));
            for (int64_t i = 0; i < N; ++i)
                futs.emplace_back(pool.addTask([]() {}));
            for (size_t i = 0; i < futs.size(); ++i)
                futs[i].wait();
        });
    }
    BENCH_SEP();

    // ---- 8. ThreadPool 提交并等待结果 ----
    {
        thrd::ThreadPool pool(4);
        BENCH_N("ThreadPool addTask + wait + get (4线程)", {
            auto fut = pool.addTask([]() { return 42; });
            fut.wait();
            volatile const int* result = fut.get();
            (void)result;
        });
    }
    BENCH_SEP();

    // ---- 9. 对比：std::thread 直接创建 ----
    {
        BENCH_N("std::thread 创建+join (参考)", {
            std::thread t([]() {});
            t.join();
        });
    }
    BENCH_SEP();

    // ================================================================
    // 高竞争场景 — 多线程争抢同一把锁
    // ================================================================

    // ---- 10. SpinLock 高竞争 ----
    {
        const int64_t N = 200000;
        const int THREADS = 4;
        thrd::SpinLock lock;
        volatile int shared_var = 0;
        auto start = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> threads;
            for (int t = 0; t < THREADS; ++t)
                threads.emplace_back([&]() {
                    for (int64_t i = 0; i < N; ++i) {
                        lock.lock();
                        shared_var++;
                        lock.unlock();
                    }
                });
            for (auto& t : threads) t.join();
        }
        auto end = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        int64_t total = THREADS * N;
        std::cout << "[BENCH] SpinLock 高竞争 (4线程×" << N << ") x" << total << ": "
                  << ns / 1000000 << " ms (" << ns / total << " ns/op)" << std::endl;
        volatile auto sink = shared_var;
        (void)sink;
    }
    BENCH_SEP();

    // ---- 11. std::mutex 高竞争（对比基准） ----
    {
        const int64_t N = 500000;
        const int THREADS = 4;
        std::mutex mtx;
        volatile int shared_var = 0;
        auto start = std::chrono::steady_clock::now();
        {
            std::vector<std::thread> threads;
            for (int t = 0; t < THREADS; ++t)
                threads.emplace_back([&]() {
                    for (int64_t i = 0; i < N; ++i) {
                        std::lock_guard<std::mutex> lk(mtx);
                        shared_var++;
                    }
                });
            for (auto& t : threads) t.join();
        }
        auto end = std::chrono::steady_clock::now();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        int64_t total = THREADS * N;
        std::cout << "[BENCH] std::mutex 高竞争 (4线程×" << N << ") x" << total << ": "
                  << ns / 1000000 << " ms (" << ns / total << " ns/op)" << std::endl;
        volatile auto sink = shared_var;
        (void)sink;
    }
    BENCH_SEP();

    BENCH_SEP();
}

} // namespace _meidothrdbench

#endif // BENCH_THREAD_HPP
