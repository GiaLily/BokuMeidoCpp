/*  bokumeidocpp库的内存管理工具，包括 ObjectPool 等  */
#pragma once
#ifndef MEM_HPP_BOKUMEIDOCPP
#define MEM_HPP_BOKUMEIDOCPP

#include <mutex>
#include <new>
#include <type_traits>
#include <vector>

#include "base.hpp"
#include "log.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace mem
{
    /*  固定容量的对象池，线程安全
        - T 必须可析构（std::is_destructible<T>）
        - construct 时检查参数能否构造 T（std::is_constructible<T, Args...>）
        - 容量在构造时固定，池满时 construct 返回 nullptr
        - 不支持拷贝、移动  */
    template <class T>
    class ObjectPool final
    {
    public:
        // 构造时 SFINAE：T 必须可析构
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && std::is_destructible<U>::value && !std::is_array<U>::value, int>::type = 0>
        explicit ObjectPool(size_t capacity);

        /*  从池中获取一个槽位并在其上构造对象
            - SFINAE：Args... 必须能构造 T
            - 池满时返回 nullptr
            - T 构造失败（抛出异常）时，异常被静默吞掉并返回 nullptr
            @param args: 转发给 T 的构造参数
            @return T* 或 nullptr  */
        template <class... Args, typename std::enable_if<std::is_constructible<T, Args...>::value, int>::type = 0>
        T* construct(Args&&... args);

        /*  析构对象并归还槽位到池中
            - 会调用 T 的析构函数
            - obj 必须是由本池 construct 返回的指针
            @return 0: 成功，-1: obj 为 nullptr 或不属于本池  */
        int destroy(T* obj);

        // 池的总容量
        size_t capacity() const;
        // 当前剩余空闲槽数
        size_t available() const;

        ObjectPool(const ObjectPool&) = delete;
        ObjectPool& operator=(const ObjectPool&) = delete;

    private:
        struct Slot;

        std::vector<Slot> slots_;
        std::vector<size_t> free_list_;
        mutable std::mutex mtx_;
    };

}    // namespace mem










/*--------------------------------------------内部实现--------------------------------------------*/


namespace mem
{

    template <class T>
    struct ObjectPool<T>::Slot
    {
        typename std::aligned_storage<sizeof(T), alignof(T)>::type storage;
    };

    template <class T>
    template <class U, typename std::enable_if<std::is_same<U, T>::value && std::is_destructible<U>::value && !std::is_array<U>::value, int>::type>
    inline ObjectPool<T>::ObjectPool(size_t capacity)
    {
        if (capacity == 0)
        {
            MEIDO_WARN("ObjectPool capacity is 0, all construct() will return nullptr");
        }
        slots_.resize(capacity);
        free_list_.reserve(capacity);
        for (size_t i = 0; i < capacity; ++i)
        {
            free_list_.push_back(capacity - 1 - i); /* 逆序入栈，pop_back O(1) */
        }
    }

    template <class T>
    template <class... Args, typename std::enable_if<std::is_constructible<T, Args...>::value, int>::type>
    inline T* ObjectPool<T>::construct(Args&&... args)
    {
        size_t idx;
        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (free_list_.empty())
                return nullptr;
            idx = free_list_.back();
            free_list_.pop_back();
        }
        // 锁外构造：避免 T 的构造函数拉长临界区，与 destroy 的锁外析构对齐
        try
        {
            return new (&slots_[idx].storage) T(std::forward<Args>(args)...);
        }
        catch (...)
        {
            // 库不抛异常：静默吞掉并归还槽位（free_list_ 容量已 reserve，push_back 不会分配内存）
            std::lock_guard<std::mutex> lk(mtx_);
            free_list_.push_back(idx);
            return nullptr;
        }
    }

    template <class T>
    inline int ObjectPool<T>::destroy(T* obj)
    {
        Slot* slot_ptr = reinterpret_cast<Slot*>(obj);
        Slot* slots_begin = slots_.data();
        Slot* slots_end = slots_begin + slots_.size();

        if (obj == nullptr || slot_ptr < slots_begin || slot_ptr >= slots_end)
        {
            MEIDO_WARN("ObjectPool::destroy called with pointer:{} not belonging to this pool", (void*)obj);
            return -1;
        }

        obj->~T();
        {
            std::lock_guard<std::mutex> lk(mtx_);
            size_t idx = slot_ptr - slots_begin;
            free_list_.push_back(idx);
        }
        return 0;
    }

    template <class T>
    inline size_t ObjectPool<T>::capacity() const
    {
        return slots_.size();
    }

    template <class T>
    inline size_t ObjectPool<T>::available() const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return free_list_.size();
    }

}    // namespace mem
}    // namespace meido

#endif    // !MEM_HPP_BOKUMEIDOCPP
