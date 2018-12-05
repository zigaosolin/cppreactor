#ifndef REACTOR_SCHEDULER_HPP_INCLUDED
#define REACTOR_SCHEDULER_HPP_INCLUDED

#include <iostream>
#include <experimental\coroutine>
#include "reactor_coroutine.hpp"

namespace reactor
{

	class next_frame
	{

	public:
		bool await_ready() const noexcept 
		{
			std::cout << "Await ready" << std::endl;
			return false;
		}

		bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
		{
			std::cout << "Await suspend" << std::endl;
			m_awaitingCoroutine = awaitingCoroutine;
			return true;
		}

		decltype(auto) await_resume()
		{
			std::cout << "Await resume" << std::endl;
		}

	private:
		std::experimental::coroutine_handle<> m_awaitingCoroutine;

	};

}

#endif