/*  thread 模块单元测试
    分类：锁(SpinLock/ReadWriteLock) | 线程池(ThreadPool/TaskFuture)  */
#pragma once
#ifndef THREAD_TEST_HPP_BOKUMEIDOCPP
#define THREAD_TEST_HPP_BOKUMEIDOCPP

#include "bokumeido/core/thread.hpp"
#include <memory>

using namespace meido;
namespace _meidothreadcheck
{

inline void SpinLockTest()
{
    thrd::SpinLock splk;
    char strs[] = "Hello World";

    bool func1_check_ret = true;
    auto func1 = [&strs, &splk, &func1_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = splk.lockScope();
            strs[0] = 'W';
            strs[1] = 'o';
            strs[2] = 'r';
            std::this_thread::yield();
            strs[3] = 'l';
            strs[4] = 'd';
            func1_check_ret = func1_check_ret && (std::string("World World") == strs);
        }
    };

    bool func2_check_ret = true;
    auto func2 = [&strs, &splk, &func2_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = splk.lockScope();
            strs[0] = 'H';
            strs[1] = 'e';
            strs[2] = 'l';
            std::this_thread::yield();
            strs[3] = 'l';
            strs[4] = 'o';
            func2_check_ret = func2_check_ret && (std::string("Hello World") == strs);
        }
    };
    std::thread thd1(func1);
    std::thread thd2(func2);
    thd1.join();
    thd2.join();

    MEIDO_ASSERT(func1_check_ret && func2_check_ret);
}

inline void ReadWriteMutexTest()
{
    thrd::ReadWriteLock rwlk;
    char strs[] = "Hello World";

    bool func1_check_ret = true;
    auto func1 = [&strs, &rwlk, &func1_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = rwlk.lockReadScope();
            strs[0] = 'W';
            strs[1] = 'o';
            strs[2] = 'r';
            strs[3] = 'l';
            strs[4] = 'd';
            func1_check_ret = func1_check_ret && (std::string("World World") == strs);
        }
    };

    bool func2_check_ret = true;
    auto func2 = [&strs, &rwlk, &func2_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = rwlk.lockReadScope();
            strs[0] = 'H';
            strs[1] = 'e';
            strs[2] = 'l';
            strs[3] = 'l';
            strs[4] = 'o';
            func2_check_ret = func2_check_ret && (std::string("Hello World") == strs);
        }
    };
    {
        std::thread thd1(func1);
        std::thread thd2(func2);
        thd1.join();
        thd2.join();
        // 读-读之间不会加锁，因此两个结果中可能存在false
        if (func1_check_ret && func2_check_ret)
            MEIDO_INFO_RAW("ReadWriteMutexTest: Both read-read checks passed this iteration (data race not triggered)");
    }

    memcpy(strs, "Hello World", 11);
    bool func3_check_ret = true;
    auto func3 = [&strs, &rwlk, &func3_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = rwlk.lockWriteScope();
            strs[0] = 'W';
            strs[1] = 'o';
            strs[2] = 'r';
            strs[3] = 'l';
            strs[4] = 'd';
            func3_check_ret = func3_check_ret && (std::string("World World") == strs);
        }
    };

    bool func4_check_ret = true;
    auto func4 = [&strs, &rwlk, &func4_check_ret]() {
        for (int i = 0; i < 10000; i++)
        {
            auto guard = rwlk.lockWriteScope();
            strs[0] = 'H';
            strs[1] = 'e';
            strs[2] = 'l';
            strs[3] = 'l';
            strs[4] = 'o';
            func4_check_ret = func4_check_ret && (std::string("Hello World") == strs);
        }
    };

    {
        func1_check_ret = true;
        func4_check_ret = true;
        std::thread thd1(func1);
        std::thread thd4(func4);
        thd1.join();
        thd4.join();

        MEIDO_ASSERT(func1_check_ret && func4_check_ret);
    }

    {
        func3_check_ret = true;
        func4_check_ret = true;
        std::thread thd3(func3);
        std::thread thd4(func4);
        thd3.join();
        thd4.join();
        MEIDO_ASSERT(func3_check_ret && func4_check_ret);
    }
}

inline void ThreadPoolTest()
{
    thrd::ThreadPool thread_pool(4);
    thrd::TaskFuture<void> state1 = thread_pool.addTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); });
    auto ft = state1.toFuture();
    ft.wait();
    MEIDO_ASSERT(state1.get() == nullptr);

    thrd::TaskFuture<int> state2 = thread_pool.addTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(100)); return 1; });
    MEIDO_ASSERT(*state2.get() == 1);
}

inline void TaskFutureTest()
{
    // 默认构造的 TaskFuture 无效
    {
        thrd::TaskFuture<int> f;
        MEIDO_ASSERT(!f.valid());
        MEIDO_ASSERT(f.finished());    // 无效任务 finished() 返回 true
    }

    // 通过线程池获取有效 TaskFuture
    thrd::ThreadPool pool(4);
    auto ft = pool.addTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(50)); return 10; });
    MEIDO_ASSERT(ft.valid());
    MEIDO_ASSERT(!ft.finished());     // 任务可能尚未完成
    ft.wait();
    MEIDO_ASSERT(ft.finished());
    MEIDO_ASSERT(*ft.get() == 10);
}

inline void ThreadPoolFullTest()
{
    thrd::ThreadPool pool(2);
    // 初始状态无任务
    MEIDO_ASSERT(!pool.full());
    // 先占满所有工作线程
    auto ft1 = pool.addTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); return 1; });
    auto ft2 = pool.addTask([] { std::this_thread::sleep_for(std::chrono::milliseconds(200)); return 2; });
    // 让任务先开始执行，确认满负载
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
    MEIDO_ASSERT(pool.full());    // working(2) + queued(0) >= pool_size(2)

    // 向队列添加更多任务
    pool.addTask([] { return 3; });
    MEIDO_ASSERT(pool.full());    // working(2) + queued(1) >= pool_size(2)

    ft1.wait();
    ft2.wait();
}

inline void TaskFutureNonCopyableRetTest()
{
    thrd::ThreadPool pool(2);

    // unique_ptr 是不可拷贝（仅可移动）的类型，验证 TaskFuture 能正常工作
    auto ft = pool.addTask([]() -> std::unique_ptr<int> {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return std::unique_ptr<int>(new int(21));
    });

    MEIDO_ASSERT(ft.valid());
    ft.wait();
    MEIDO_ASSERT(ft.finished());
    const auto* p = ft.get();
    MEIDO_ASSERT(p != nullptr);
    // *p 为 const unique_ptr<int>&，**p 为 const int&
    MEIDO_ASSERT(**p == 21);

    // 默认构造的 TaskFuture 对 move-only 类型同样有效
    thrd::TaskFuture<std::unique_ptr<int>> empty_ft;
    MEIDO_ASSERT(!empty_ft.valid());
    MEIDO_ASSERT(empty_ft.get() == nullptr);
}

// 不可拷贝、不可移动赋值、仅支持移动构造的类型
struct MoveCtorOnlyRet
{
    int val;
    explicit MoveCtorOnlyRet(int v) : val(v) {}
    MoveCtorOnlyRet(const MoveCtorOnlyRet&) = delete;
    MoveCtorOnlyRet& operator=(const MoveCtorOnlyRet&) = delete;
    MoveCtorOnlyRet(MoveCtorOnlyRet&&) = default;
    MoveCtorOnlyRet& operator=(MoveCtorOnlyRet&&) = delete;
};

inline void TaskFutureMoveCtorOnlyRetTest()
{
    thrd::ThreadPool pool(2);

    auto ft = pool.addTask([]() -> MoveCtorOnlyRet {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        return MoveCtorOnlyRet(30);
    });

    MEIDO_ASSERT(ft.valid());
    ft.wait();
    MEIDO_ASSERT(ft.finished());
    const auto* p = ft.get();
    MEIDO_ASSERT(p != nullptr);
    MEIDO_ASSERT(p->val == 30);

    // 默认构造也应当有效
    thrd::TaskFuture<MoveCtorOnlyRet> empty_ft;
    MEIDO_ASSERT(!empty_ft.valid());
    MEIDO_ASSERT(empty_ft.get() == nullptr);
}

inline void check()
{
    MEIDO_INFO_RAW("\n--------------------check thrd start--------------------");

    // ---- 1. 锁测试 ----
    SpinLockTest();
    ReadWriteMutexTest();

    // ---- 2. 线程池测试 ----
    ThreadPoolTest();
    TaskFutureTest();
    ThreadPoolFullTest();
    TaskFutureNonCopyableRetTest();
    TaskFutureMoveCtorOnlyRetTest();

    MEIDO_INFO_RAW("---------------------check thrd end---------------------\n\n");
}

}    // namespace _meidothreadcheck

#endif    // !THREAD_TEST_HPP_BOKUMEIDOCPP
