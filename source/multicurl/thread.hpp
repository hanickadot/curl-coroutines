#ifndef MULTICURL_THREAD_HPP
#define MULTICURL_THREAD_HPP

#include <thread>
#include <coroutine>

namespace toolbox {

struct switch_to_other_thread {
	std::thread backup_thread;

	constexpr bool await_ready() const {
		return false;
	}
	template <typename P> constexpr void await_suspend(std::coroutine_handle<P> current) {
		auto & promise = current.promise();
		auto backup_thread = std::move(promise.thread);
		promise.thread = promise.run_in_thread();
	}
	constexpr void await_resume() {
		backup_thread.join();
	}
};

template <typename T> struct different_thread_task {
	struct promise_type {
		std::optional<T> result;
		std::thread thread;

		auto initial_suspend() const noexcept { return std::suspend_always{}; }
		auto final_suspend() const noexcept { return std::suspend_always{}; }
		void return_value(std::convertible_to<T> auto && val) {
			result = std::move(val);
		}
		void unhandled_exception() noexcept { }
		auto get_return_object() {
			return std::coroutine_handle<promise_type>::from_promise(*this);
		}

		auto run_in_thread() {
			return std::thread{[h = std::coroutine_handle<promise_type>::from_promise(*this)] {
				std::cout << "new thread: " << std::this_thread::get_id() << "\n";
				h.resume();
			}};
		}
	};

	different_thread_task(std::coroutine_handle<promise_type> h) noexcept: handle{h} {
		promise().thread = promise().run_in_thread();
	}

	operator T &() & {
		return *promise().result;
	}

	operator T() && {
		return std::move(*promise().result);
	}

	bool await_ready() {
		return promise().result.has_value();
	}

	template <typename P> auto await_suspend(std::coroutine_handle<P> current) -> std::coroutine_handle<P> {
		promise().thread.join();
		return current;
	}

	T await_resume() {
		return std::move(*(promise().result));
	}

	~different_thread_task() {
		if (handle) {
			if (promise().thread.joinable()) {
				promise().thread.join();
			}
			handle.destroy();
		}
	}

protected:
	promise_type & promise() {
		return handle.promise();
	}

	std::coroutine_handle<promise_type> handle{nullptr};
};

} // namespace toolbox

#endif
