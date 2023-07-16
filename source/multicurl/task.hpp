#ifndef MULTICURL_TASK_HPP
#define MULTICURL_TASK_HPP

#include <iostream>
#include <optional>
#include <coroutine>

namespace toolbox {

template <typename T> struct task {
	struct promise_type {
		std::coroutine_handle<> previous;

		std::optional<T> data;
		std::exception_ptr exception{nullptr};

		template <std::convertible_to<T> Y> void return_value(Y && value) noexcept {
			data = std::move(value);
		}

		auto get_return_object() noexcept {
			return std::coroutine_handle<promise_type>::from_promise(*this);
		}

		void unhandled_exception() {
			exception = std::current_exception();
		}

		auto initial_suspend() const noexcept { return std::suspend_never{}; }
		auto final_suspend() const noexcept {
			struct jump_to_other_coroutine {
				bool await_ready() const noexcept { return false; }
				void await_resume() const noexcept { }
				auto await_suspend(std::coroutine_handle<promise_type> handle) -> std::coroutine_handle<> {
					const auto previous = handle.promise().previous;
					if (previous == nullptr) {
						return std::noop_coroutine();
					}
					return previous;
				}
			};
			return jump_to_other_coroutine{};
		}
	};

	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle;

	task(handle_type h): handle{h} { }

	bool await_ready() const noexcept {
		return handle.done();
	}

	void await_suspend(std::coroutine_handle<> coroutine) const noexcept {
		handle.promise().previous = coroutine;
	}

	T await_resume() const {
		if (auto eptr = handle.promise().exception) {
			std::rethrow_exception(eptr);
		}
		assert(handle.promise().data.has_value());
		return std::move(*handle.promise().data);
	}

	T get() {
		if (auto eptr = handle.promise().exception) {
			std::rethrow_exception(eptr);
		}
		assert(handle.promise().data.has_value());
		return std::move(*handle.promise().data);
	}

	operator T() {
		return get();
	}
};

template <> struct task<void> {
	struct promise_type {
		std::coroutine_handle<> previous;
		std::exception_ptr exception{nullptr};

		void return_void() noexcept { }

		auto get_return_object() noexcept {
			return std::coroutine_handle<promise_type>::from_promise(*this);
		}

		void unhandled_exception() {
			exception = std::current_exception();
		}

		auto initial_suspend() const noexcept { return std::suspend_never{}; }
		auto final_suspend() const noexcept {
			struct jump_to_other_coroutine {
				bool await_ready() const noexcept { return false; }
				void await_resume() const noexcept { }
				auto await_suspend(std::coroutine_handle<promise_type> handle) -> std::coroutine_handle<> {
					const auto previous = handle.promise().previous;
					if (previous == nullptr) {
						// in case there is no previous ... just stop the chain (because we are the top coroutine)
						return std::noop_coroutine();
					}
					return previous;
				}
			};
			return jump_to_other_coroutine{};
		}
	};

	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle;

	task(handle_type h): handle{h} { }

	bool await_ready() const noexcept {
		return handle.done();
	}

	void await_suspend(std::coroutine_handle<> coroutine) const noexcept {
		handle.promise().previous = coroutine;
	}

	void await_resume() const {
		if (auto eptr = handle.promise().exception) {
			std::rethrow_exception(eptr);
		}
	}

	void get() {
		if (auto eptr = handle.promise().exception) {
			std::rethrow_exception(eptr);
		}
	}
};

} // namespace toolbox

#endif
