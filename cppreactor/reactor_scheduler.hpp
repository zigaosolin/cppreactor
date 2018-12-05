#ifndef REACTOR_SCHEDULER_HPP_INCLUDED
#define REACTOR_SCHEDULER_HPP_INCLUDED

#include <iostream>
#include <experimental\coroutine>
#include <vector>
#include "reactor_coroutine.hpp"


namespace reactor
{
	class scheduler
	{
	public:
		void update(float delta_time)
		{
			for (auto& handle : m_next_frame)
			{
				handle.resume();
			}

			m_next_frame.clear();
		}

		void enqueue_update(std::experimental::coroutine_handle<> handle)
		{
			m_next_frame.push_back(handle);
		}

		void enqueue(reactor_coroutine& coroutine)
		{
			coroutine.update(0);
		}

	private:
		std::vector<std::experimental::coroutine_handle<> > m_next_frame;
	};


	class next_frame
	{

	public:
		next_frame(scheduler& scheduler)
			: m_scheduler(scheduler)
		{
		}

		bool await_ready() const noexcept 
		{
			std::cout << "Await ready" << std::endl;
			return false;
		}

		bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
		{
			std::cout << "Await suspend" << std::endl;
			m_awaitingCoroutine = awaitingCoroutine;
			m_scheduler.enqueue_update(m_awaitingCoroutine);
			return true;
		}

		decltype(auto) await_resume()
		{
			std::cout << "Await resume" << std::endl;
		}

	private:
		std::experimental::coroutine_handle<> m_awaitingCoroutine;
		scheduler& m_scheduler;

	};

}

#endif