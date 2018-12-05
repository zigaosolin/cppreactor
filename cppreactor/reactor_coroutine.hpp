#ifndef REACTOR_COROUTINE_HPP_INCLUDED
#define REACTOR_COROUTINE_HPP_INCLUDED

#include <iostream>
#include <experimental/coroutine>
#include <type_traits>
#include <utility>
#include <exception>

namespace reactor
{
	class reactor_coroutine;

	namespace detail
	{
		class reactor_coroutine_promise
		{
		public:
			reactor_coroutine_promise() = default;

			reactor_coroutine get_return_object() noexcept;

			constexpr std::experimental::suspend_always initial_suspend() const 
			{ 
				return {}; 
			}
			constexpr std::experimental::suspend_always final_suspend() const 
			{
				return {};
			}

			template<
				typename U,
				typename = std::enable_if_t<std::is_same<U, T>::value>>
				std::experimental::suspend_always yield_value(U& value) noexcept
			{
				std::cout << "Yield value<U,U> " << value << std::endl;
				m_value = std::addressof(value);
				return {};
			}


			void unhandled_exception()
			{
				std::cout << "Exception " << std::endl;

				m_exception = std::current_exception();
			}

			void return_void()
			{
				std::cout << "Return void " << std::endl;
			}

			/*
			template<typename U>
			std::experimental::suspend_always await_transform(U&& value)
			{
				std::cout << "Await transform " << std::endl;

				return value;
			}*/

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:

			std::exception_ptr m_exception;

		};
	}

	// A coroutine that is handled by scheduler
	class reactor_coroutine
	{
	public:

		using promise_type = detail::reactor_coroutine_promise;

		reactor_coroutine() noexcept
			: m_coroutine(nullptr)
		{}

		reactor_coroutine(reactor_coroutine&& other) noexcept
			: m_coroutine(other.m_coroutine)
		{
			other.m_coroutine = nullptr;
		}

		reactor_coroutine(const reactor_coroutine& other) = delete;

		~reactor_coroutine()
		{
			if (m_coroutine)
			{
				m_coroutine.destroy();
			}
		}

		reactor_coroutine& operator=(reactor_coroutine other) noexcept
		{
			swap(other);
			return *this;
		}

		void swap(reactor_coroutine& other) noexcept
		{
			std::swap(m_coroutine, other.m_coroutine);
		}

		bool update(float dt)
		{
			if (m_coroutine)
			{
				m_coroutine.resume();
				if (!m_coroutine.done())
				{
					return true;
				}

				m_coroutine.promise().rethrow_if_exception();
			}

			return false;
		}
	private:

		friend class detail::reactor_coroutine_promise;

		explicit reactor_coroutine(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		std::experimental::coroutine_handle<promise_type> m_coroutine;
	};

	void swap(reactor_coroutine& a, reactor_coroutine& b)
	{
		a.swap(b);
	}

	namespace detail
	{
		reactor_coroutine reactor_coroutine_promise::get_return_object() noexcept
		{
			using coroutine_handle = std::experimental::coroutine_handle<reactor_coroutine_promise>;
			return reactor_coroutine{ coroutine_handle::from_promise(*this) };
		}
	}
}

#endif