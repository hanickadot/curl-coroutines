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

	void associate(easycurl & request, std::coroutine_handle<> coroutine) {
		auto [it, success] = associated_coroutines.emplace(request.native_handle(), coroutine);

		assert(success);
		std::cout << "> add handle to multicurl...\n";
		handle.add_handle(request);
	}

	auto select_next() -> std::coroutine_handle<> {
		std::cout << "> select_next() size = " << associated_coroutines.size() << "\n";
		for (;;) {
			const auto opt_count = handle.perform();

			std::cout << "> perform = " << *opt_count << "\n";
			// check if something ended...
			for (;;) {
				[[maybe_unused]] int msgs_in_queue = 0;
				struct CURLMsg * m = curl_multi_info_read(handle.native_handle(), &msgs_in_queue);

				if (m && m->msg == CURLMSG_DONE) {
					std::cout << "> coroutine selected!\n";
					const auto it = associated_coroutines.find(m->easy_handle);

					assert(it != associated_coroutines.end());

					return it->second;
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
	multicurl_scheduler scheduler{};

	std::optional<T> result{std::nullopt};

	// jumps to caller and awaiter :)
	std::coroutine_handle<> caller{nullptr};
	std::coroutine_handle<> awaiter{nullptr};

	auto self() -> std::coroutine_handle<fetch_promise> {
		return std::coroutine_handle<fetch_promise>::from_promise(*this);
	}

	auto initial_suspend() {
		struct store_caller_in_promise {
			fetch_promise & promise;
			bool await_ready() {
				return false;
			}
			bool await_suspend(std::coroutine_handle<> caller) {
				std::cout << "storing caller... " << caller.address() << "\n";
				promise.caller = caller;
				return false;
			}
			bool await_suspend(std::coroutine_handle<fetch_promise>) = delete;
			void await_resume() { }
		};
		return store_caller_in_promise{*this};
	}

	auto await_transform(with_multicurl tag) {
		struct register_in_multicurl_and_jump_to_caller {
			easycurl & request_handle;
			fetch_promise & promise;

			bool await_ready() {
				return false;
			}

			auto await_suspend(std::coroutine_handle<fetch_promise> caller) {
				promise.scheduler.associate(request_handle, promise.self());
				std::cout << "jumping back to caller... " << promise.caller.address() << ", self = " << promise.self().address() << "\n";

				// we got caller stored in initia_suspend
				assert(promise.caller != nullptr);
				return promise.caller;
			}

			void await_resume() { }
		};
		return register_in_multicurl_and_jump_to_caller{tag.handle, *this};
	}

	auto final_suspend() noexcept {
		struct conditional_suspend {
			fetch_promise & promise;
			bool await_ready() noexcept {
				return false;
			}
			auto await_suspend(std::coroutine_handle<> myself) noexcept -> std::coroutine_handle<> {
				if (promise.awaiter != nullptr) {
					std::cout << "we have someone waiting for us...\n";
					// if have awaiting caller, just jump there
					return promise.awaiter;
				} else {
					std::cout << "multicurl looks for next one...\n";
					// TODO multicurl to select next coroutine (as there is no one waiting for me, yet)
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

	T & get_result() & {
		assert(result.has_value());
		return *result;
	}

	T get_result() && {
		assert(result.has_value());
		return std::move(*result);
	}

	template <std::convertible_to<T> Y> void return_value(Y && value) {
		value = std::move(value);
	}

	void unhandled_exception() noexcept {
		std::terminate();
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

	operator T &() & requires has_get_result<promise_type> {
		return promise().get_result();
	}

	operator T() && requires has_get_result<promise_type> {
		return promise().get_result();
	}

	bool await_ready() const noexcept {
		return promise().result.has_value();
	}

	auto await_suspend(std::coroutine_handle<> awaiter_for_result) -> std::coroutine_handle<> {
		// we know we need to suspend, which means we didn't finished yet
		// we need to run multi_curl_process to let the coroutine finish
		assert(handle.promise().awaiter == nullptr);
		handle.promise().awaiter = awaiter_for_result;

		std::cout << "awaiting for result...\n";
		// TODO select next coroutine with multicurl to jump to, there must be some maybe this task as otherwise we wouldn't suspend

		return promise().scheduler.select_next();
	}

	T await_resume() noexcept {
		return std::move(promise().get_result());
	}
};

} // namespace toolbox

#endif
