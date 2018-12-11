#ifndef REACTOR_COROUTINE_HPP_INCLUDED
#define REACTOR_COROUTINE_HPP_INCLUDED

#include <iostream>
#include <experimental/coroutine>
#include <type_traits>
#include <utility>
#include <exception>

namespace reactor
{

	class reactor_scheduler;
	class reactor_coroutine;
	class next_frame;


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

			template<typename U>
			std::experimental::suspend_always await_transform(U&& value)
			{
				std::cout << "Await transform " << std::endl;

				return value;
			}

			next_frame await_transform(next_frame&& awaitable);

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:
			friend class reactor_coroutine;

			reactor_scheduler* m_scheduler;
			std::exception_ptr m_exception;

		};
	}


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

		next_frame next_frame() noexcept;

	private:

		friend class detail::reactor_coroutine_promise;
		friend class reactor_scheduler;

		explicit reactor_coroutine(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		void schedule(reactor_scheduler& scheduler)
		{
			auto& p = m_coroutine.promise();
			assert(p.m_scheduler == nullptr);
			p.m_scheduler = &scheduler;
		}

		bool update_next_frame(float data)
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

		std::experimental::coroutine_handle<promise_type> m_coroutine;
	};

	class reactor_scheduler
	{
	public:
		void update(float frame_data)
		{		
			// Sets current frame data, members with access can return it
			m_frame_data = frame_data;

			for (auto& start_coroutine : m_start_coroutines)
			{
				start_coroutine->update_next_frame(frame_data);
			}
			m_start_coroutines.clear();

			m_frames.swap();

			for (auto& handle : m_frames.front())
			{
				handle.resume();
			}

			m_frames.front().clear();
		}

		void enqueue(reactor_coroutine& coroutine)
		{
			coroutine.schedule(*this);
			m_start_coroutines.push_back(&coroutine);
		}

	private:
		friend class next_frame;

		void enqueue_update(std::experimental::coroutine_handle<> handle)
		{
			m_frames.back().push_back(handle);
		}

		struct double_buffer
		{
			bool m_frame1_front = false;
			std::vector<std::experimental::coroutine_handle<> > m_next_frame1;
			std::vector<std::experimental::coroutine_handle<> > m_next_frame2;

			std::vector<std::experimental::coroutine_handle<> >& front()
			{
				return m_frame1_front ? m_next_frame1 : m_next_frame2;
			}

			std::vector<std::experimental::coroutine_handle<> >& back()
			{
				return m_frame1_front ? m_next_frame2 : m_next_frame1;
			}

			void swap()
			{
				m_frame1_front = !m_frame1_front;
			}
		};

		double_buffer m_frames;
		std::vector<reactor_coroutine*> m_start_coroutines;
		float m_frame_data;


	};

	class next_frame
	{

	public:
		next_frame()
			: m_scheduler(nullptr)
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
			m_scheduler->enqueue_update(m_awaitingCoroutine);
			return true;
		}

		decltype(auto) await_resume()
		{
			std::cout << "Await resume" << std::endl;
			return m_scheduler->m_frame_data;
		}

	private:
		friend class detail::reactor_coroutine_promise;

		std::experimental::coroutine_handle<> m_awaitingCoroutine;
		reactor_scheduler* m_scheduler;

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

		next_frame reactor_coroutine_promise::await_transform(next_frame&& awaitable)
		{
			assert(m_scheduler != nullptr);
			awaitable.m_scheduler = m_scheduler;

			return awaitable;
		}
	}
}

#endif