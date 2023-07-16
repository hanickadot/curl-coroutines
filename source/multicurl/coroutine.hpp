#ifndef MULTICURL_COROUTINES_HPP
#define MULTICURL_COROUTINES_HPP

#include "multicurl.hpp"
#include <map>
#include <coroutine>

namespace toolbox {

struct noop_awaitable {
	bool await_ready() {
		return true;
	}
	template <typename Y> auto await_suspend(std::coroutine_handle<Y> current_task) {
		// will never be called
		return current_task;
	}
	void await_resume() const noexcept { }
};

struct just_start_fetch {
	easycurl handle;
	bool & finished;
};

struct all_fetchs_to_finish {
};

template <typename T> struct multicurl_fetch_task {
	struct promise_type;

	std::coroutine_handle<promise_type> handle;

	struct promise_type {
		// T result;
		multicurl multihandle{};
		std::map<void *, std::pair<bool &, std::coroutine_handle<promise_type>>> coroutines_to_resume{};
		std::coroutine_handle<promise_type> everything_finished{};

		auto initial_suspend() noexcept { return std::suspend_never{}; }
		auto final_suspend() noexcept { return std::suspend_always{}; }

		auto get_return_object() noexcept {
			return multicurl_fetch_task{std::coroutine_handle<promise_type>::from_promise(*this)};
		}

		void return_void() { }
		// void return_value(T && value) { result = std::move(value); }

		void unhandled_exception() { }

		void add_handle(easycurl & handle) {
			multihandle.add_handle(handle);
		}

		void add_only_resume_handle(easycurl & handle, bool & finished, std::coroutine_handle<promise_type> coroutine) {
			coroutines_to_resume.emplace(handle.native_handle(), std::pair{finished, coroutine});
		}

		auto process_handles_and_return_first_finished() -> std::coroutine_handle<promise_type> {
			for (;;) {
				const auto opt_count = multihandle.perform();

				// check if something ended...
				for (;;) {
					[[maybe_unused]] int msgs_in_queue = 0;
					struct CURLMsg * m = curl_multi_info_read(multihandle.native_handle(), &msgs_in_queue);

					if (m && m->msg == CURLMSG_DONE) {
						const auto it = coroutines_to_resume.find(m->easy_handle);

						assert(it != coroutines_to_resume.end());

						it->second->first = true;

						if (it->second->second == nullptr) {
							continue;
						}

						return it->second->second;
					} else {
						break;
					}
				}

				// nothing to do...
				assert(opt_count.has_value());
				assert(*opt_count > 0u);

				multihandle.poll();
			}
		}

		struct easy_awaitable {
			promise_type & promise;
			easycurl & handle;

			bool await_ready() const noexcept { return false; }
			auto await_suspend(std::coroutine_handle<promise_type> current_task) -> std::coroutine_handle<promise_type> {
				promise.add_handle(handle, current_task);
				return promise.process_handles_and_return_first_finished();
			}
			void await_resume() const noexcept { }
		};

		auto await_transform(just_start_fetch & start) -> noop_awaitable {
			multihandle.add_handle(start.handle);
			return {};
		}

		// auto await_transform(easycurl & easy) noexcept {
		//	return easy_awaitable{*this, easy};
		// }
	};
};

template <typename T> struct awaitable_fetcher {
	easycurl handle{};
	bool started{false};
	bool finished{false};
	T result{};

	awaitable_fetcher(awaitable_fetcher &&) = delete;
	awaitable_fetcher(const awaitable_fetcher &) = delete;
	awaitable_fetcher & operator=(awaitable_fetcher &&) = delete;
	awaitable_fetcher & operator=(const awaitable_fetcher &&) = delete;

	awaitable_fetcher(std::string_view url) {
		handle.url(url);
		handle.write_function([](awaitable_fetcher & dwn, std::span<const std::byte> in) { dwn.write(in); }, *this);
	}

	void write(std::span<const std::byte> in) {
		static_assert(sizeof(typename T::value_type) == sizeof(std::byte));
		const auto input = std::span(reinterpret_cast<const typename T::value_type *>(in.data()), in.size());

		result.reserve(result.size() + input.size());

		std::copy(input.begin(), input.end(), std::back_inserter(result));
	}

	// this will start fetching but won't block coroutine
	auto start() {
		if (!started) {
			started = true;
			return just_start_fetch(handle, finished);
		}
	}

	bool await_ready() const noexcept {
		return false;
		// return handle.ready;
	}

	template <typename Y> auto await_suspend(std::coroutine_handle<Y> current_task) {
		auto & promise = current_task.promise();
		if (!started) {
			promise.add_handle(handle);
		}

		promise.add_resume_handle(handle, current_task);

		return promise.process_handles_and_return_first_finished();
	}

	// template <typename Y> auto await_suspend(Y &&) -> std::noop_coroutine_handle {
	//	static_assert(toolbox::always_false<Y>, "you can fetch only inside multicurl based coroutines");
	//	return std::noop_coroutine();
	// }

	T await_resume() noexcept {
		return std::move(result);
	}
};

template <typename T> auto fetch(std::string_view url) {
	return awaitable_fetcher<T>(url);
}

template <typename T = std::vector<std::byte>> auto fetch_binary(std::string_view url) {
	return fetch<T>(url);
}

template <typename T = std::string> auto fetch_text(std::string_view url) {
	return fetch<T>(url);
}

} // namespace toolbox

#endif
