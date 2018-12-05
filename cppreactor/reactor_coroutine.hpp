#ifndef REACTOR_COROUTINE_HPP_INCLUDED
#define REACTOR_COROUTINE_HPP_INCLUDED

#include <experimental/coroutine>
#include <type_traits>
#include <utility>
#include <exception>

namespace reactor
{
	template<typename T>
	class reactor_coroutine;

	namespace detail
	{
		template<typename T>
		class reactor_coroutine_promise
		{
		public:

			using value_type = std::remove_reference_t<T>;
			using reference_type = std::conditional_t<std::is_reference_v<T>, T, T&>;
			using pointer_type = value_type * ;

			reactor_coroutine_promise() = default;

			reactor_coroutine<T> get_return_object() noexcept;

			constexpr std::experimental::suspend_always initial_suspend() const { return {}; }
			constexpr std::experimental::suspend_always final_suspend() const { return {}; }

			template<
				typename U,
				typename = std::enable_if_t<std::is_same<U, T>::value>>
				std::experimental::suspend_always yield_value(U& value) noexcept
			{
				m_value = std::addressof(value);
				return {};
			}

			std::experimental::suspend_always yield_value(T&& value) noexcept
			{
				m_value = std::addressof(value);
				return {};
			}

			void unhandled_exception()
			{
				m_value = nullptr;
				m_exception = std::current_exception();
			}

			void return_void()
			{
				m_value = nullptr;
			}

			reference_type value() const noexcept
			{
				return *m_value;
			}

			// Don't allow any use of 'co_await' inside the coroutine.
			template<typename U>
			std::experimental::suspend_never await_transform(U&& value) = delete;

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:

			pointer_type m_value;
			std::exception_ptr m_exception;

		};
	}

	// A coroutine that is handled by scheduler
	template<typename T>
	class reactor_coroutine
	{
	public:

		using promise_type = detail::reactor_coroutine_promise<T>;

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

		friend class detail::reactor_coroutine_promise<T>;

		explicit reactor_coroutine(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		std::experimental::coroutine_handle<promise_type> m_coroutine;
	};

	template<typename T>
	void swap(reactor_coroutine<T>& a, reactor_coroutine<T>& b)
	{
		a.swap(b);
	}

	namespace detail
	{
		template<typename T>
		reactor_coroutine<T> reactor_coroutine_promise<T>::get_return_object() noexcept
		{
			using coroutine_handle = std::experimental::coroutine_handle<reactor_coroutine_promise<T>>;
			return reactor_coroutine<T>{ coroutine_handle::from_promise(*this) };
		}
	}
}

#endif