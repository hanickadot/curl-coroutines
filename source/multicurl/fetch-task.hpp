#ifndef MULTICURL_FETCH_TASK_HPP
#define MULTICURL_FETCH_TASK_HPP

#include "multicurl.hpp"
#include "task.hpp"
#include <map>
#include <coroutine>

namespace toolbox {

struct multicurl_scheduler {
	std::map<void *, std::coroutine_handle<>> associated_coroutines{};
	multicurl handle{};

	multicurl_scheduler() {
		handle.enable_multiplexing();
	}

	void associate(easycurl & request, std::coroutine_handle<> coroutine) {
		auto [it, success] = associated_coroutines.emplace(request.native_handle(), coroutine);

		assert(success);
		handle.add_handle(request);
	}

	auto select_next() -> std::coroutine_handle<> {
		for (;;) {
			const auto opt_count = handle.perform();

			// std::cout << "> perform = " << *opt_count << "\n";
			//  check if something ended...
			for (;;) {
				[[maybe_unused]] int msgs_in_queue = 0;
				struct CURLMsg * m = curl_multi_info_read(handle.native_handle(), &msgs_in_queue);

				if (m && m->msg == CURLMSG_DONE) {
					const auto it = associated_coroutines.find(m->easy_handle);

					assert(it != associated_coroutines.end());

					const auto next_coroutine = it->second;

					associated_coroutines.erase(it);
					curl_multi_remove_handle(handle.native_handle(), m->easy_handle);

					return next_coroutine;
				} else {
					break;
				}
			}

			// nothing to do...
			assert(opt_count.has_value());
			// assert(*opt_count > 0u);

			handle.poll();
		}
	}
};

auto get_global_multicurl_scheduler() -> multicurl_scheduler & {
	static auto global_scheduler = multicurl_scheduler{};
	return global_scheduler;
}

struct with_multicurl {
	easycurl & handle;

	with_multicurl(easycurl & h): handle{h} { }
};

template <typename T> struct fetch_promise {
	multicurl_scheduler & scheduler = get_global_multicurl_scheduler();

	std::optional<T> result{std::nullopt};
	std::exception_ptr exception{nullptr};

	// jumps to caller and awaiter :)
	std::coroutine_handle<> awaiter{nullptr};

	auto self() -> std::coroutine_handle<fetch_promise> {
		return std::coroutine_handle<fetch_promise>::from_promise(*this);
	}

	auto initial_suspend() {
		return std::suspend_never{};
	}

	auto await_transform(with_multicurl tag) {
		// just associate and pause here
		scheduler.associate(tag.handle, self());
		// and go back to caller
		return std::suspend_always();
	}

	auto final_suspend() noexcept {
		struct conditional_suspend {
			fetch_promise & promise;
			bool await_ready() noexcept {
				return false;
			}
			auto await_suspend(std::coroutine_handle<> myself) noexcept -> std::coroutine_handle<> {
				if (promise.awaiter != nullptr) {
					// if have awaiting caller, just jump there
					return promise.awaiter;
				} else {
					// if not, look into the download scheduler for something else
					return promise.scheduler.select_next();
				}
			}
			void await_resume() noexcept { }
		};
		// to avoid destroying result
		return conditional_suspend{*this};
	}

	auto get_return_object() {
		return self();
	}

	template <std::convertible_to<T> Y> void return_value(Y && value) {
		result = std::move(value);
	}

	void unhandled_exception() noexcept {
		exception = std::current_exception();
	}
};

template <typename T> struct fetch_task {
	static_assert(!std::same_as<T, void>);

	using promise_type = fetch_promise<T>;
	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle{};

	fetch_task(handle_type h): handle{h} { }

	promise_type & promise() {
		return handle.promise();
	}

	const promise_type & promise() const {
		return handle.promise();
	}

	bool await_ready() const noexcept {
		// if result is there, we won't suspend
		return handle.done();
	}

	auto await_suspend(std::coroutine_handle<> awaiter_for_result) -> std::coroutine_handle<> {
		// make sure no-one else already waits for this task
		assert(handle.promise().awaiter == nullptr);
		handle.promise().awaiter = awaiter_for_result;

		// because the result is still not there, we need to schedule something else...
		return promise().scheduler.select_next();
	}

	T await_resume() {
		if (auto eptr = handle.promise().exception) {
			std::rethrow_exception(eptr);
		}
		// and result is here
		return std::move(*promise().result);
	}
};

} // namespace toolbox

#endif
