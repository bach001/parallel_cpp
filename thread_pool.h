/*
 * thread_pool.h
 *
 *  Created on: Oct 28, 2021
 *      Author: bach
 */

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include "safe_queue.h"

class thread_pool {
private:
	class thread_worker // 内置线程工作类
	{
	private:
		thread_pool *m_pool; // 所属线程池
		size_t m_id; // 工作id
	public:
		// 构造函数
		thread_worker(thread_pool *pool, const int id) :
				m_pool(pool), m_id(id) {
		}

		// 重载()操作
		void operator()() {
			std::function<void()> func; // 定义基础函数类func

			bool dequeued; // 是否正在取出队列中元素

			while (!m_pool->m_shutdown) {
				// 为线程环境加锁，互访问工作线程的休眠和唤醒
				std::unique_lock<std::mutex> lock(m_pool->m_mutex);

				// 如果任务队列为空，阻塞当前线程
				if (m_pool->m_queue.empty()) {
					m_pool->m_cond_v.wait(lock); // 等待条件变量通知，开启线程
				}

				// 取出任务队列中的元素
				dequeued = m_pool->m_queue.dequeue(func);

				// 如果成功取出，执行工作函数
				if (dequeued)
					func();
			}
		}
	};

	safe_queue<std::function<void()>> m_queue; // 执行函数安全队列，即任务队列
	std::mutex m_mutex; // 线程休眠锁互斥变量
	std::condition_variable m_cond_v; // 线程环境锁，可以让线程处于休眠或者唤醒状态

	std::vector<std::thread> m_threads; // 工作线程队列
	bool m_shutdown; // 线程池是否关闭

public:
	// 线程池构造函数
	thread_pool(const int n_threads = 8) :
			m_threads(std::vector<std::thread>(n_threads)), m_shutdown(false) {
	}

	thread_pool(const thread_pool&) = delete;
	thread_pool(thread_pool&&) = delete;

	thread_pool& operator=(const thread_pool&) = delete;
	thread_pool& operator=(thread_pool&&) = delete;

	// Inits thread pool
	void init() {
		for (size_t i = 0; i < m_threads.size(); ++i) {
			m_threads.at(i) = std::thread(thread_worker(this, i)); // 分配工作线程
		}
	}

	// Waits until threads finish their current task and shutdowns the pool
	void shutdown() {
		m_shutdown = true;
		m_cond_v.notify_all(); // 通知，唤醒所有工作线程

		for (size_t i = 0; i < m_threads.size(); ++i) {
			if (m_threads.at(i).joinable()) // 判断线程是否在等待
			{
				m_threads.at(i).join(); // 将线程加入到等待队列
			}
		}
	}

	// Submit a function to be executed asynchronously by the pool
	template<typename F, typename ... Args>
	auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))>
	{
		// Create a function with bounded parameter ready to execute
		std::function<decltype(f(args...))()> func = std::bind(
				std::forward<F>(f), std::forward<Args>(args)...); // 连接函数和参数定义，特殊函数类型，避免左右值错误

		// Encapsulate it into a shared pointer in order to be able to copy construct
		auto task_ptr = std::make_shared<
				std::packaged_task<decltype(f(args...))()>>(func);

		// wrap packaged task into void function
		std::function<void()> wrap_func = [task_ptr]() {
			(*task_ptr)();
		};

		// 队列通用安全封包函数，并压入安全队列
		m_queue.enqueue(wrap_func);

		// 唤醒一个等待中的线程
		m_cond_v.notify_one();

		// 返回先前注册的任务指针
		return task_ptr->get_future();
	}
};

#endif

