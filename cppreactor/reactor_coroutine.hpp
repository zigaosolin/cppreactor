#ifndef REACTOR_COROUTINE_HPP_INCLUDED
#define REACTOR_COROUTINE_HPP_INCLUDED

#include <experimental/coroutine>
#include <type_traits>
#include <utility>
#include <exception>

namespace cppcoro
{
	struct reactor_default_frame_data
	{
		float delta_time;
	};

	template <class T = reactor_default_frame_data>
	class reactor_scheduler;

	template <class T = reactor_default_frame_data>
	class reactor_coroutine;

	template <class R, class T = reactor_default_frame_data>
	class reactor_coroutine_return;

	template <class T = reactor_default_frame_data>
	class next_frame;


	namespace detail
	{
		template <class T>
		class coroutine_awaitable;

		template <class R, class T>
		class coroutine_awaitable_return;

		template <class T = reactor_default_frame_data>
		class reactor_coroutine_promise
		{
		public:
			reactor_coroutine_promise()
				: m_awaiter(nullptr), m_scheduler(nullptr)
			{
			}

			reactor_coroutine<T> get_return_object() noexcept;

			constexpr std::experimental::suspend_always initial_suspend() const
			{
				return {};
			}
			constexpr std::experimental::suspend_always final_suspend() const
			{
				return {};
			}


			void unhandled_exception()
			{
				m_exception = std::current_exception();
			}

			void return_void()
			{
				if (m_awaiter)
				{
					m_awaiter->m_awaitingCoroutine.resume();
					m_awaiter = nullptr;
				}
				return;
			}

			template<typename U>
			std::experimental::suspend_always await_transform(U&& value)
			{
				return value;
			}

			next_frame<T> await_transform(next_frame<T>&& awaitable);
			coroutine_awaitable<T> await_transform(reactor_coroutine<T>&& awaitable);

			template <typename U>
			coroutine_awaitable_return<U, T> await_transform(reactor_coroutine_return<U, T>&& awaitable);

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:
			friend class reactor_coroutine<T>;
			friend class coroutine_awaitable<T>;
			
			reactor_scheduler<T>* m_scheduler;
			std::exception_ptr m_exception;
			coroutine_awaitable<T>* m_awaiter;
		};

		template <class R, class T = reactor_default_frame_data>
		class reactor_coroutine_promise_return
		{
		public:
			reactor_coroutine_promise_return()
				: m_awaiter(nullptr), m_scheduler(nullptr)
			{
			}

			reactor_coroutine_return<R, T> get_return_object() noexcept;

			constexpr std::experimental::suspend_always initial_suspend() const
			{
				return {};
			}
			constexpr std::experimental::suspend_always final_suspend() const
			{
				return {};
			}

			void unhandled_exception()
			{
				m_exception = std::current_exception();
			}

			void return_value(R value)
			{
				m_value = value;
				if (m_awaiter)
				{
					m_awaiter->m_awaitingCoroutine.resume();
					m_awaiter = nullptr;
				}
			}

			R get_value()
			{
				return m_value;
			}

			template<typename U>
			std::experimental::suspend_always await_transform(U&& value)
			{
				return value;
			}

			next_frame<T> await_transform(next_frame<T>&& awaitable);
			coroutine_awaitable<T> await_transform(reactor_coroutine<T>&& awaitable);

			template <class U>
			coroutine_awaitable_return<U, T> await_transform(reactor_coroutine_return<U, T>&& awaitable);

			void rethrow_if_exception()
			{
				if (m_exception)
				{
					std::rethrow_exception(m_exception);
				}
			}

		private:
			friend class reactor_coroutine_return<R, T>;
			friend class coroutine_awaitable_return<R, T>;

			reactor_scheduler<T>* m_scheduler;
			std::exception_ptr m_exception;
			R m_value;
			coroutine_awaitable_return<R, T>* m_awaiter;
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
		friend class detail::coroutine_awaitable<T>;
		friend class reactor_scheduler<T>;

		explicit reactor_coroutine(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		void schedule(reactor_scheduler<T>& scheduler)
		{
			auto& p = m_coroutine.promise();

			// False means that coroutine was already scheduled by something else, not permited in this model due to efficiency
			assert(p.m_scheduler == nullptr); 
			p.m_scheduler = &scheduler;
		}

		bool update_next_frame()
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

	template <class R, class T>
	class reactor_coroutine_return
	{
	public:

		using promise_type = detail::reactor_coroutine_promise_return<R, T>;

		reactor_coroutine_return() noexcept
			: m_coroutine(nullptr)
		{}

		reactor_coroutine_return(reactor_coroutine_return&& other) noexcept
			: m_coroutine(other.m_coroutine)
		{
			other.m_coroutine = nullptr;
		}

		reactor_coroutine_return(const reactor_coroutine_return& other) = delete;

		reactor_coroutine_return& operator=(reactor_coroutine_return other) noexcept
		{
			swap(other);
			return *this;
		}

		void swap(reactor_coroutine_return& other) noexcept
		{
			std::swap(m_coroutine, other.m_coroutine);
		}

	private:

		friend class detail::reactor_coroutine_promise_return<R, T>;
		friend class detail::coroutine_awaitable_return<R, T>;

		explicit reactor_coroutine_return(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
			: m_coroutine(coroutine)
		{}

		void schedule(reactor_scheduler<T>& scheduler)
		{
			auto& p = m_coroutine.promise();

			// False means that coroutine was already scheduled by something else, not permited in this model due to efficiency
			assert(p.m_scheduler == nullptr);
			p.m_scheduler = &scheduler;
		}

		bool update_next_frame()
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
		void update_next_frame(T reactor_default_frame_data)
		{		
			// Sets current frame data, members with access can return it
			m_reactor_default_frame_data = reactor_default_frame_data;

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
				start_coroutine->update_next_frame();
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
		friend class detail::coroutine_awaitable<T>;

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
		
		T m_reactor_default_frame_data;
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
			return false;
		}

		bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
		{
			m_awaitingCoroutine = awaitingCoroutine;
			m_scheduler->enqueue_update(m_awaitingCoroutine);
			return true;
		}

		decltype(auto) await_resume()
		{
			return m_scheduler->m_reactor_default_frame_data;
		}

	private:
		friend class detail::reactor_coroutine_promise<T>;

		template <class U, class V>
		friend class detail::reactor_coroutine_promise_return;

		std::experimental::coroutine_handle<> m_awaitingCoroutine;
		reactor_scheduler<T>* m_scheduler;

	};

	namespace detail
	{
		template <class T>
		class coroutine_awaitable
		{

		public:
			coroutine_awaitable(reactor_scheduler<T>& scheduler, reactor_coroutine<T>& coroutine)
				: m_scheduler(&scheduler), m_coroutine(coroutine)
			{
			}

			bool await_ready() const noexcept
			{
				return false;
			}

			bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
			{
				auto& promise = m_coroutine.m_coroutine.promise();
				assert(promise.m_awaiter == nullptr);
				promise.m_awaiter = this;

				m_awaitingCoroutine = awaitingCoroutine;

				m_coroutine.schedule(*m_scheduler);
				return m_coroutine.update_next_frame();
			}

			decltype(auto) await_resume()
			{
				auto& promise = m_coroutine.m_coroutine.promise();
				promise.rethrow_if_exception();

				return;
			}

		private:
			friend class reactor_coroutine_promise<T>;

			reactor_coroutine<T>& m_coroutine;
			reactor_scheduler<T>* m_scheduler;
			std::experimental::coroutine_handle<> m_awaitingCoroutine;

		};

		template <class R, class T>
		class coroutine_awaitable_return
		{

		public:
			coroutine_awaitable_return(reactor_scheduler<T>& scheduler, reactor_coroutine_return<R, T>& coroutine)
				: m_scheduler(&scheduler), m_coroutine(coroutine)
			{
			}

			bool await_ready() const noexcept
			{
				return false;
			}

			bool await_suspend(std::experimental::coroutine_handle<> awaitingCoroutine)
			{
				auto& promise = m_coroutine.m_coroutine.promise();
				assert(promise.m_awaiter == nullptr);
				promise.m_awaiter = this;

				m_awaitingCoroutine = awaitingCoroutine;

				m_coroutine.schedule(*m_scheduler);
				return m_coroutine.update_next_frame();
			}

			decltype(auto) await_resume()
			{
				auto& promise = m_coroutine.m_coroutine.promise();

				promise.rethrow_if_exception();

				return promise.get_value();
			}

		private:
			friend class reactor_coroutine_promise_return<R, T>;

			reactor_coroutine_return<R, T>& m_coroutine;
			reactor_scheduler<T>* m_scheduler;
			std::experimental::coroutine_handle<> m_awaitingCoroutine;
		};
	}



	template <class T = reactor_default_frame_data>
	void swap(reactor_coroutine<T>& a, reactor_coroutine<T>& b)
	{
		a.swap(b);
	}

	template <class R, class T = reactor_default_frame_data>
	void swap(reactor_coroutine_return<R,T>& a, reactor_coroutine_return<R,T>& b)
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

		template <class T>
		coroutine_awaitable<T> reactor_coroutine_promise<T>::await_transform(reactor_coroutine<T>&& awaitable)
		{
			assert(m_scheduler != nullptr);			
			return coroutine_awaitable{ *m_scheduler, awaitable };
		}

		template <class T>
		template <class U>
		coroutine_awaitable_return<U, T> reactor_coroutine_promise<T>::await_transform(reactor_coroutine_return<U, T>&& awaitable)
		{
			assert(m_scheduler != nullptr);
			return coroutine_awaitable_return<U,T>{ *m_scheduler, awaitable };
		}


		template <class R, class T>
		reactor_coroutine_return<R,T> reactor_coroutine_promise_return<R,T>::get_return_object() noexcept
		{
			using coroutine_handle = std::experimental::coroutine_handle<reactor_coroutine_promise_return<R,T> >;
			return reactor_coroutine_return{ coroutine_handle::from_promise(*this) };
		}

		template <class R, class T>
		next_frame<T> reactor_coroutine_promise_return<R, T>::await_transform(next_frame<T>&& awaitable)
		{
			assert(m_scheduler != nullptr);
			awaitable.m_scheduler = m_scheduler;

			return awaitable;
		}

		template <class R, class T>
		coroutine_awaitable<T> reactor_coroutine_promise_return<R, T>::await_transform(reactor_coroutine<T>&& awaitable)
		{
			assert(m_scheduler != nullptr);
			return coroutine_awaitable{ *m_scheduler, awaitable };
		}

		template <class R, class T>
		template <class U>
		coroutine_awaitable_return<U, T> reactor_coroutine_promise_return<R, T>::await_transform(reactor_coroutine_return<U, T>&& awaitable)
		{
			assert(m_scheduler != nullptr);
			return coroutine_awaitable_return{ *m_scheduler, awaitable };
		}
	}
}

#endif