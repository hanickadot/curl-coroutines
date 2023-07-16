#ifndef MULTICURL_FETCH_HPP
#define MULTICURL_FETCH_HPP

#include "multicurl.hpp"
#include <map>
#include <queue>
#include <thread>
#include <coroutine>

namespace toolbox {

struct curl {
	multicurl handle;

	auto fetch_text(std::string_view url) {
	}

	auto fetch_binary(std::string_view url) {
	}
};

template <typename R> struct curl_request {
	easycurl handler{};
	R result{};
	bool ready{false};

	curl_request(std::string_view url) noexcept {
		handler.url(url);
	}

	bool await_ready() const {
		return ready;
	}

	template <typename P> auto await_suspend(std::coroutine_handle<P> current) -> std::coroutine_handle<> {
		auto & scheduler = current.promise().scheduler;
		return current;
	}

	R await_resume() {
		return std::move(result);
	}
};

struct scheduler_t {
	std::queue<std::coroutine_handle<>> ready;
};

auto get_global_scheduler() -> scheduler_t & {
	static auto global_scheduler = scheduler_t{};
	return global_scheduler;
}

template <typename T> concept awaitable = requires(T & object, const T & const_object) {
	{ const_object.await_ready() } -> std::same_as<bool>;
};

template <typename T, typename Scheduler>
concept await_transformable_with = requires(Scheduler & sch, T && obj) {
	{ sch.await_transform(std::forward<T>(obj)) } -> awaitable;
};

template <typename T>
struct immediate_task_promise;

template <typename T> struct immediate_task {
	friend struct immediate_task_promise<T>;
	using promise_type = immediate_task_promise<T>;

	immediate_task(std::coroutine_handle<promise_type> h) noexcept: handle{h} { }

	T & get_result() & {
		return promise().result;
	}

	T get_result() && {
		return std::move(promise().result);
	}

	operator T &() & {
		return get_result();
	}

	operator T() && {
		return get_result();
	}

	~immediate_task() noexcept {
		if (handle) {
			handle.destroy();
		}
	}

private:
	promise_type & promise() {
		return handle.promise();
	}

	std::coroutine_handle<promise_type> handle;
};

template <typename T> struct immediate_task_promise {
	scheduler_t & scheduler;
	T result;

	immediate_task_promise(scheduler_t & sch = get_global_scheduler()) noexcept: scheduler{sch} { }

	auto initial_suspend() noexcept { return std::suspend_never{}; }
	auto final_suspend() noexcept { return std::suspend_always{}; }

	void return_value(std::convertible_to<T> auto && r) noexcept { result = std::move(r); }
	auto get_return_object() noexcept {
		return immediate_task<T>{std::coroutine_handle<immediate_task_promise>::from_promise(*this)};
	}
	void unhandled_exception() const noexcept {
		try {
			throw;
		} catch (const std::function<void(scheduler_t &)> & fnc) {
			fnc(scheduler);
		}
	}
};

template <> struct immediate_task<void> {
	friend struct immediate_task_promise<void>;
	using promise_type = immediate_task_promise<void>;

	immediate_task(promise_type & pr) noexcept: promise{pr} { }

private:
	promise_type & promise;
};

auto fetch_text(std::string_view url) {
	return curl_request<std::string>(url);
}

} // namespace toolbox

#endif
