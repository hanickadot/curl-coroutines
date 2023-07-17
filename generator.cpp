#include <exception>
#include <iostream>
#include <thread>
#include <coroutine>

template <typename T> struct generator {
	struct promise_type {
		T current_value{};
		std::exception_ptr exception{nullptr};

		auto initial_suspend() { return std::suspend_always{}; }
		auto final_suspend() noexcept { return std::suspend_always{}; }
		auto get_return_object() {
			return std::coroutine_handle<promise_type>::from_promise(*this);
		}
		void unhandled_exception() {
			std::terminate();
		}
		auto yield_value(T value) {
			current_value = value;
			return std::suspend_always{};
		}
	};

	using handle_type = std::coroutine_handle<promise_type>;

	handle_type handle = nullptr;

	generator(handle_type h) noexcept: handle{h} { }

	generator(generator && other) noexcept: handle{std::exchange(other.handle, nullptr)} { }
	generator(const generator & other) = delete;

	~generator() {
		if (handle) {
			handle.destroy();
		}
	}

	struct iterator {
		handle_type handle;

		iterator(handle_type h): handle{h} {
			h.resume();
		}

		iterator & operator++() {
			handle.resume();
			return *this;
		}

		T operator*() {
			auto & promise = handle.promise();
			if (promise.exception) {
				std::rethrow_exception(promise.exception);
			}
			return promise.current_value;
		}
	};

	struct sentinel {
		friend constexpr bool operator==(iterator it, sentinel) {
			return it.handle.done();
		}
	};

	iterator begin() {
		return iterator{handle};
	}

	sentinel end() {
		return {};
	}
};

auto fibonnaci() -> generator<uint64_t> {
	// 0, 1, 1, 2, 3, 5, 8, 13, 21, 34
	uint64_t a = 0;
	uint64_t b = 1;

	for (;;) {
		co_yield a;
		a = std::exchange(b, a + b);
		co_await wake_me_up_somewhere_else();
	}
}

int main() {
	// auto f = fibonnaci();
	for (auto i: fibonnaci()) {
		if (i > 1000) {
			break;
		}
		std::cout << i << "\n";
	}
}