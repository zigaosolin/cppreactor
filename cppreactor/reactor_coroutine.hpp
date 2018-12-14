#ifndef REACTOR_COROUTINE_HPP_INCLUDED
#define REACTOR_COROUTINE_HPP_INCLUDED

#include <experimental/coroutine>
#include <type_traits>
#include <utility>
#include <exception>

namespace reactor
{
	struct frame_data
	{
		float delta_time;
	};

	template <class T = frame_data>
	class reactor_scheduler;

	template <class T = frame_data>
	class reactor_coroutine;

	template <class T = frame_data>
	class next_frame;


	namespace detail
	{
		template <class T = frame_data>
		class reactor_coroutine_promise
		{
		public:
			reactor_coroutine_promise() = default;

			reactor_coroutine<T> get_return_object() noexcept;

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
				//std::cout << "Yield value<U,U> " << value << std::endl;
				m_value = std::addressof(value);
				return {};
			}


			void unhandled_exception()
			{
				//std::cout << "Exception " << std::endl;

				m_exception = std::current_exception();
			}

			void return_void()
			{
				//std::cout << "Return void " << std::endl;
			}

			template<typename U>
			std::experimental::suspend_always await_transform(U&& value)
			{
				//std::cout << "Await transform " << std::endl;

				return value;
			}

			next_frame<T> await_transform(next_frame<T>&& awaitable);

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:
			friend class reactor_coroutine<T>;

			reactor_scheduler<T>* m_scheduler;
			std::exception_ptr m_exception;

		};
	}


	template <class T>
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

	private:

		friend class detail::reactor_coroutine_promise<T>;
		friend class reactor_scheduler<T>;

		explicit reactor_coroutine(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		void schedule(reactor_scheduler<T>& scheduler)
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

	template <class T>
	class reactor_scheduler
	{
	public:
		void update_next_frame(float frame_data)
		{		
			// Sets current frame data, members with access can return it
			m_frame_data = frame_data;

			m_frames.swap();

			for (auto& handle : m_frames.front())
			{
				handle.resume();
			}

			m_frames.front().clear();

			// We start all coroutines right after updates
			m_start_coroutines.swap();

			for (auto& start_coroutine : m_start_coroutines.front())
			{
				start_coroutine->update_next_frame(frame_data);
			}
			m_start_coroutines.front().clear();
		}

		void push(reactor_coroutine<T>& coroutine)
		{
			coroutine.schedule(*this);
			m_start_coroutines.back().push_back(&coroutine);
		}

	private:
		friend class next_frame<T>;

		void enqueue_update(std::experimental::coroutine_handle<> handle)
		{
			m_frames.back().push_back(handle);
		}

		template <class D>
		struct double_buffer
		{
			std::vector<D>* m_front;
			std::vector<D>* m_back;

			std::vector<D> m_next_frame1;
			std::vector<D> m_next_frame2;

			double_buffer()
			{
				m_front = &m_next_frame1;
				m_back = &m_next_frame2;
			}

			std::vector<D>& front()
			{
				return *m_front;
			}

			std::vector<D>& back()
			{
				return *m_back;
			}

			void swap()
			{
				std::swap(m_front, m_back);
			}
		};

		double_buffer<std::experimental::coroutine_handle<> > m_frames;
		double_buffer<reactor_coroutine<T>*> m_start_coroutines;
		float m_frame_data;
	};

	template <class T>
	class next_frame
	{

	public:
		next_frame()
			: m_scheduler(nullptr)
		{
		}

		bool await_ready() const noexcept
		{
			//std::cout << "Await ready" << std::endl;
			return false;
		}

		bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
		{
			//std::cout << "Await suspend" << std::endl;
			m_awaitingCoroutine = awaitingCoroutine;
			m_scheduler->enqueue_update(m_awaitingCoroutine);
			return true;
		}

		decltype(auto) await_resume()
		{
			//std::cout << "Await resume" << std::endl;
			return m_scheduler->m_frame_data;
		}

	private:
		friend class detail::reactor_coroutine_promise<T>;

		std::experimental::coroutine_handle<> m_awaitingCoroutine;
		reactor_scheduler<T>* m_scheduler;

	};



	template <class T = frame_data>
	void swap(reactor_coroutine<T>& a, reactor_coroutine<T>& b)
	{
		a.swap(b);
	}

	namespace detail
	{
		template <class T>
		reactor_coroutine<T> reactor_coroutine_promise<T>::get_return_object() noexcept
		{
			using coroutine_handle = std::experimental::coroutine_handle<reactor_coroutine_promise<T> >;
			return reactor_coroutine{ coroutine_handle::from_promise(*this) };
		}

		template <class T>
		next_frame<T> reactor_coroutine_promise<T>::await_transform(next_frame<T>&& awaitable)
		{
			assert(m_scheduler != nullptr);
			awaitable.m_scheduler = m_scheduler;

			return awaitable;
		}
	}
}

#endif