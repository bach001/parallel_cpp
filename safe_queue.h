/*
 * safe_queue.h
 *
 *  Created on: Oct 28, 2021
 *      Author: bach
 */

#ifndef SAFE_QUEUE_H_
#define SAFE_QUEUE_H_

#include "sys_lib.h"

// Thread safe implementation of a Queue using a std::queue
template <typename T>
class safe_queue
{
    std::queue<T> m_queue; //利用模板函数构造队列
    std::mutex m_mutex; // 访问互斥信号量

public:
    safe_queue() {}
    safe_queue(safe_queue &&other) {}
    ~safe_queue() {}

    bool empty() // 返回队列是否为空
    {
        std::unique_lock<std::mutex> lk(m_mutex); // 互斥信号变量加锁，防止m_queue被改变
        return m_queue.empty();
    }

    int size()
    {
        std::unique_lock<std::mutex> lk(m_mutex); // 互斥信号变量加锁，防止m_queue被改变
        return m_queue.size();
    }

    // 队列添加元素
    void enqueue(T &t)
    {
        std::unique_lock<std::mutex> lk(m_mutex);
        m_queue.emplace(t);
    }

    // 队列取出元素
    bool dequeue(T &t)
    {
        std::unique_lock<std::mutex> lk(m_mutex); // 队列加锁
        if (m_queue.empty())
            return false;
        t = std::move(m_queue.front()); // 取出队首元素，返回队首元素值，并进行右值引用
        m_queue.pop(); // 弹出入队的第一个元素

        return true;
    }
};

#endif /* SAFE_QUEUE_H_ */
