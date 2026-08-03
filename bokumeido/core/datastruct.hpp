/*  实现一些标准库没有的数据结构  */
#pragma once
#ifndef DATASTRUCT_HPP_BOKUMEIDOCPP
#define DATASTRUCT_HPP_BOKUMEIDOCPP

#include <type_traits>
#include <vector>

#include "base.hpp"
#include "log.hpp"


namespace meido
{
/*--------------------------------------------用户接口--------------------------------------------*/

namespace ds
{
    /*  循环队列
        - T不能有const、volatile或引用符号修饰
        - T不可为void
        - T必须存在无参构造函数
        - T至少可由自身的左值或右值之一赋值  */
    template <class T>
    class CircularQueue final
    {
    public:
        // 构造一个容量为0的空对象
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && _priv::CircularQueueElemChecker<U>::value, int>::type = 0>
        CircularQueue() {}

        /*  构造循环队列；模板参数仅用于类型检查，不可修改
            @param queue_capacity: 循环队列的容量，为0时入队出队都无效  */
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && _priv::CircularQueueElemChecker<U>::value, int>::type = 0>
        CircularQueue(size_t queue_capacity);

        /*  作为左值入队
            - T能够由自身的左值赋值时，接口实例化
            - 模板参数仅用于类型检查，不可修改
            @param value: 要入队的元素  */
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && std::is_assignable<U&, const U&>::value, int>::type = 0>
        void enqueue(const T& value);

        /*  作为右值入队
            - T能够由自身的右值赋值时，接口实例化
            - 模板参数仅用于类型检查，不可修改
            @param value: 要入队的元素  */
        template <class U = T, typename std::enable_if<std::is_same<U, T>::value && std::is_assignable<U&, U&&>::value, int>::type = 0>
        void enqueue(T&& value);

        /*  尝试出队，队列为空时出队失败
            @param dst: 用于接收出队的元素，出队成功时将被赋值
            @return 成功时返回true，队列为空时返回false  */
        bool tryDequeue(T& dst);

        // 队列是否为空
        bool empty() const;
        // 队列是否已满
        bool full() const;

        // 支持移动，禁止拷贝
        CircularQueue(CircularQueue<T>&& rvalue) noexcept;
        CircularQueue<T>& operator=(CircularQueue<T>&& rvalue) noexcept;

    private:
        void move(CircularQueue<T>&& rvalue) noexcept;
        template <class U, typename std::enable_if<std::is_assignable<U&, U&&>::value, int>::type = 0>
        bool tryDequeueDispatch(U& dst);
        template <class U, typename std::enable_if<!std::is_assignable<U&, U&&>::value, int>::type = 0>
        bool tryDequeueDispatch(U& dst);

        size_t cache_num_ = 0;
        std::vector<T> buffer_;
        size_t head_ = 0;
        size_t tail_ = 0;
        size_t count_ = 0;
    };

}    // namespace ds










/*--------------------------------------------内部实现--------------------------------------------*/

namespace ds
{
    template <class T>
    template <class U, typename std::enable_if<std::is_same<U, T>::value && _priv::CircularQueueElemChecker<U>::value, int>::type>
    inline CircularQueue<T>::CircularQueue(size_t queue_capacity)
    {
        cache_num_ = queue_capacity;
        buffer_.resize(queue_capacity);
        head_ = 0;
        tail_ = 0;
        count_ = 0;
    }

    template <class T>
    template <class U, typename std::enable_if<std::is_same<U, T>::value && std::is_assignable<U&, const U&>::value, int>::type>
    inline void CircularQueue<T>::enqueue(const T& value)
    {
        if (buffer_.empty())
        {
            MEIDO_DEBUG("Capacity is 0\n");
            return;
        }
        if (count_ == cache_num_)
            head_ = (head_ + 1) % cache_num_;
        else
            count_++;

        buffer_[tail_] = value;
        tail_ = (tail_ + 1) % cache_num_;
    }

    template <class T>
    template <class U, typename std::enable_if<std::is_same<U, T>::value && std::is_assignable<U&, U&&>::value, int>::type>
    inline void CircularQueue<T>::enqueue(T&& value)
    {
        if (buffer_.empty())
        {
            MEIDO_DEBUG("Capacity is 0\n");
            return;
        }
        if (count_ == cache_num_)
            head_ = (head_ + 1) % cache_num_;
        else
            count_++;

        buffer_[tail_] = std::move(value);
        tail_ = (tail_ + 1) % cache_num_;
    }

    template <class T>
    inline bool CircularQueue<T>::tryDequeue(T& dst)
    {
        if (buffer_.empty())
        {
            MEIDO_DEBUG("Capacity is 0\n");
            return false;
        }
        return this->tryDequeueDispatch(dst);
    }

    template <class T>
    inline bool CircularQueue<T>::empty() const
    {
        return count_ == 0;
    }

    template <class T>
    inline bool CircularQueue<T>::full() const
    {
        return count_ == cache_num_;
    }

    template <class T>
    inline CircularQueue<T>::CircularQueue(CircularQueue<T>&& rvalue) noexcept
    {
        this->move(std::move(rvalue));
    }
    template <class T>
    inline CircularQueue<T>& CircularQueue<T>::operator=(CircularQueue<T>&& rvalue) noexcept
    {
        if (this != &rvalue)
            this->move(std::move(rvalue));
        return *this;
    }

    template <class T>
    inline void CircularQueue<T>::move(CircularQueue<T>&& rvalue) noexcept
    {
        buffer_ = std::move(rvalue.buffer_);
        cache_num_ = rvalue.cache_num_;
        head_ = rvalue.head_;
        tail_ = rvalue.tail_;
        count_ = rvalue.count_;

        rvalue.cache_num_ = 0;
        rvalue.head_ = 0;
        rvalue.tail_ = 0;
        rvalue.count_ = 0;
    }

    template <class T>
    template <class U, typename std::enable_if<std::is_assignable<U&, U&&>::value, int>::type>
    inline bool CircularQueue<T>::tryDequeueDispatch(U& dst)
    {
        if (this->empty())
            return false;
        dst = std::move(buffer_[head_]);
        head_ = (head_ + 1) % cache_num_;
        count_--;
        return true;
    }

    template <class T>
    template <class U, typename std::enable_if<!std::is_assignable<U&, U&&>::value, int>::type>
    inline bool CircularQueue<T>::tryDequeueDispatch(U& dst)
    {
        if (this->empty())
            return false;
        dst = buffer_[head_];
        head_ = (head_ + 1) % cache_num_;
        count_--;
        return true;
    }

}    // namespace ds
}    // namespace meido

#endif