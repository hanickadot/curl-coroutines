#ifndef MULTICURL_TASK_HPP
#define MULTICURL_TASK_HPP

#include <iostream>
#include <optional>
#include <coroutine>

namespace toolbox {

template <typename T> struct result_storage {
	std::optional<T> result{std::nullopt};

	template <std::convertible_to<T> Y> void return_value(Y && arg) {
		result = std::forward<Y>(arg);
	}

	T & get_result() & noexcept {
		assert(result.has_value());
		return *result;
	}

	T get_result() && noexcept {
		assert(result.has_value());
		return *result;
	}
};

template <typename T> concept has_get_result = requires(T & ref, T && temporary) {
	ref.get_result();
	temporary.get_result();
};

template <> struct result_storage<void> {
	void return_void() noexcept {
	}
};

template <typename T> struct task_initial_suspend {
	T initial_suspend() {
		return {};
	}
};

template <typename T> struct task_final_suspend {
	T final_suspend() noexcept {
		return {};
	}
};

template <typename... Pieces> struct task_promise: Pieces... {
	task_promise(): Pieces{}... { }

	auto get_return_object() {
		return std::coroutine_handle<task_promise>::from_promise(*this);
	}

	void unhandled_exception() noexcept {
	}
};

template <typename T> struct immediate_task {
	using promise_type = task_promise<result_storage<T>, task_initial_suspend<std::suspend_never>, task_final_suspend<std::suspend_always>>;
	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle{};

	immediate_task(handle_type h): handle{h} { }

	promise_type & promise() {
		return handle.promise();
	}

	operator T &() & requires has_get_result<promise_type> {
		return promise().get_result();
	}

	operator T() && requires has_get_result<promise_type> {
		return promise().get_result();
	}
};

struct hana_suspend {
	bool await_ready() {
		return false;
	}
	std::coroutine_handle<> await_suspend(std::coroutine_handle<> handle) {
		std::cout << handle.address() << "\n";
		return handle;
	}
	void await_resume() {
		return;
	}
};

template <typename T> struct special_task {
	using promise_type = task_promise<result_storage<T>, task_initial_suspend<hana_suspend>, task_final_suspend<std::suspend_always>>;
	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle{};

	special_task(handle_type h): handle{h} { }

	promise_type & promise() {
		return handle.promise();
	}

	operator T &() & requires has_get_result<promise_type> {
		return promise().get_result();
	}

	operator T() && requires has_get_result<promise_type> {
		return promise().get_result();
	}
};

} // namespace toolbox

#endif
