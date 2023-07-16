#ifndef MULTICURL_MULTICURL_HPP
#define MULTICURL_MULTICURL_HPP

#include <iostream>
#include <ostream>
#include <span>
#include <string_view>
#include <utility/concepts.hpp>
#include <utility>
#include <curl/curl.h>

namespace toolbox {

struct empty_type { };

template <typename...> constexpr auto always_false = false;

constexpr auto empty_value_of_empty_type = empty_type{};

struct info: std::string_view {
	using super = std::string_view;
	using super::super;

	friend auto operator<<(std::ostream & os, info i) -> std::ostream & {
		return os << "info: " << static_cast<std::string_view>(i);
	}
};

struct header_out: std::string_view {
	using super = std::string_view;
	using super::super;

	friend auto operator<<(std::ostream & os, header_out i) -> std::ostream & {
		return os << "header out: " << static_cast<std::string_view>(i) << "\n";
	}
};

struct data_out: std::span<const std::byte> {
	using super = std::span<const std::byte>;
	using super::super;

	friend auto operator<<(std::ostream & os, data_out i) -> std::ostream & {
		return os;
	}
};

struct ssl_data_out: std::span<const std::byte> {
	using super = std::span<const std::byte>;
	using super::super;

	friend auto operator<<(std::ostream & os, ssl_data_out i) -> std::ostream & {
		return os;
	}
};

struct header_in: std::string_view {
	using super = std::string_view;
	using super::super;

	friend auto operator<<(std::ostream & os, header_in i) -> std::ostream & {
		return os << "header in: " << static_cast<std::string_view>(i) << "\n";
	}
};

struct data_in: std::span<const std::byte> {
	using super = std::span<const std::byte>;
	using super::super;

	friend auto operator<<(std::ostream & os, data_in i) -> std::ostream & {
		return os;
	}
};

struct ssl_data_in: std::span<const std::byte> {
	using super = std::span<const std::byte>;
	using super::super;

	friend auto operator<<(std::ostream & os, ssl_data_in i) -> std::ostream & {
		return os;
	}
};

constexpr auto custom_trace_function = [](auto && item) {
	std::cout << item;
};

struct callback_helper {
	template <typename F> static size_t debug(CURL * handle, curl_infotype type, char * data, size_t size, void * userdata) {
		F & fnc = *static_cast<F *>(userdata);

		switch (type) {
		case CURLINFO_TEXT:
			fnc(info(data, size));
			break;
		case CURLINFO_HEADER_OUT:
			fnc(header_out(data, size));
			break;
		case CURLINFO_DATA_OUT:
			fnc(data_out(reinterpret_cast<const std::byte *>(data), size));
			break;
		case CURLINFO_SSL_DATA_OUT:
			fnc(ssl_data_out(reinterpret_cast<const std::byte *>(data), size));
			break;
		case CURLINFO_HEADER_IN:
			fnc(header_in(data, size));
			break;
		case CURLINFO_DATA_IN:
			fnc(data_in(reinterpret_cast<const std::byte *>(data), size));
			break;
		case CURLINFO_SSL_DATA_IN:
			fnc(ssl_data_in(reinterpret_cast<const std::byte *>(data), size));
			break;
		default:
			break;
		}

		return 0;
	}

	template <typename F> static size_t write(char * ptr, size_t size, size_t nmemb, void * userdata) noexcept(noexcept(std::invoke(std::declval<F &>(), std::declval<std::span<const std::byte>>()))) {
		try {
			F & fnc = *static_cast<F *>(userdata);
			const auto data = std::span<const std::byte>{reinterpret_cast<std::byte *>(ptr), size * nmemb};

			fnc(data);

			return size * nmemb;
		} catch (...) {
			// TODO store the exception somewhere
			return (std::numeric_limits<size_t>::max)();
		}
	}

	template <typename F> static size_t header(char * ptr, size_t size, size_t nmemb, void * userdata) noexcept(noexcept(std::invoke(std::declval<F &>(), std::declval<std::string_view>()))) {
		try {
			F & fnc = *static_cast<F *>(userdata);
			const auto data = std::string_view{ptr, size * nmemb};

			fnc(data);

			return size * nmemb;
		} catch (...) {
			// TODO store the exception somewhere
			return (std::numeric_limits<size_t>::max)();
		}
	}
};

class easycurl {
	void * handle{nullptr};

public:
	easycurl();
	inline easycurl(easycurl && other) noexcept: handle{std::exchange(other.handle, nullptr)} { }
	easycurl(const easycurl &) = delete;

	inline easycurl & operator=(easycurl && other) noexcept {
		std::swap(other.handle, handle);
		return *this;
	}

	easycurl & operator=(easycurl & other) = delete;

	~easycurl();

	template <typename F> inline void debug_function(F & fnc) noexcept {
		curl_easy_setopt(native_handle(), CURLOPT_VERBOSE, 1L);
		curl_easy_setopt(native_handle(), CURLOPT_DEBUGFUNCTION, callback_helper::debug<F>);
		curl_easy_setopt(native_handle(), CURLOPT_DEBUGDATA, static_cast<const void *>(&fnc));
	}

	template <std::default_initializable CB, typename T> inline void write_function(CB &&, T & obj) noexcept
		requires(std::is_invocable_v<CB, T &, std::span<const std::byte>>)
	{
		const auto helper_function = +[](char * ptr, size_t size, size_t nmemb, void * udata) -> size_t {
			T & obj = *static_cast<T *>(udata);

			try {
				CB{}(obj, std::span<const std::byte>(reinterpret_cast<std::byte *>(ptr), size * nmemb));
				return size * nmemb;
			} catch (...) {
				// in case of exception propagate the error as transfer error :(
				// TODO: do something about it
				return (std::numeric_limits<size_t>::max)();
			}
		};

		curl_easy_setopt(native_handle(), CURLOPT_WRITEFUNCTION, helper_function);
		curl_easy_setopt(native_handle(), CURLOPT_WRITEDATA, static_cast<const void *>(&obj));
	}

	template <std::default_initializable CB> inline void write_function(CB &&) noexcept
		requires(std::is_invocable_v<CB, std::span<const std::byte>>)
	{
		// use a dummy parameter to satisfy the API
		write_function([](const empty_type &, std::span<const std::byte> in) { CB{}(in); }, empty_value_of_empty_type);
	}

	template <std::invocable<std::span<const std::byte>> F> inline void write_function(F & fnc) noexcept {
		// just pass the reference value as a parameter
		write_function([](F & fnc, std::span<const std::byte> in) { fnc(in); }, fnc);
	}

	template <typename... Args> inline void write_function(Args &&...) {
		static_assert(always_false<Args...>, "You can't use temporary lambda as `write_function` argument!");
	}

	template <std::invocable<std::string_view> F> inline void header_function(F & fnc) noexcept {
		curl_easy_setopt(native_handle(), CURLOPT_HEADERFUNCTION, callback_helper::header<F>);
		curl_easy_setopt(native_handle(), CURLOPT_HEADERDATA, static_cast<const void *>(&fnc));
	}

	inline void url(const char * value) noexcept {
		curl_easy_setopt(native_handle(), CURLOPT_URL, value);
	}

	inline void url(const std::string & value) noexcept {
		url(value.c_str());
	}

	inline void url(std::string_view value) {
		// TODO I know :(
		url(std::string(value));
	}

	inline void buffer_size(signed long sz = 1024L * 1024L) noexcept {
		curl_easy_setopt(native_handle(), CURLOPT_BUFFERSIZE, sz);
	}

	inline unsigned perform() noexcept {
		return static_cast<unsigned>(curl_easy_perform(native_handle()));
	}

	inline void force_fresh() noexcept {
		curl_easy_setopt(native_handle(), CURLOPT_FRESH_CONNECT, 1L);
	}

	inline void pipewait() {
		curl_easy_setopt(native_handle(), CURLOPT_PIPEWAIT, 1L);
	}

	inline auto get_content_length() const noexcept -> std::optional<size_t> {
		curl_off_t result{0};
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);

		if (result == -1) {
			return std::nullopt;
		}

		return static_cast<size_t>(result);
	}

	inline void * native_handle() noexcept {
		return handle;
	}

	inline const void * native_handle() const noexcept {
		return handle;
	}
};

class multicurl {
	void * handle{nullptr};
	// std::map<void *,

public:
	multicurl();
	inline multicurl(multicurl && other) noexcept: handle{std::exchange(other.handle, nullptr)} { }
	multicurl(const multicurl &) = delete;

	inline multicurl & operator=(multicurl && other) noexcept {
		std::swap(other.handle, handle);
		return *this;
	}

	multicurl & operator=(multicurl & other) = delete;

	~multicurl();

	// user api
	void * native_handle() noexcept {
		return handle;
	}

	const void * native_handle() const noexcept {
		return handle;
	}

	void add_handle(easycurl & single_request_handler) {
		const auto r = curl_multi_add_handle(native_handle(), single_request_handler.native_handle());
		std::cout << "curl_multi_add_handle -> " << curl_multi_strerror(r) << "\n";
	}

	auto perform() -> std::optional<unsigned> {
		int count = 0;

		const auto r = curl_multi_perform(native_handle(), &count);

		std::cout << "curl_multi_perform -> " << r << " (count = " << count << ")\n";

		if (r != CURLM_OK) {
			return std::nullopt;
		}

		// if (CURLM_OK != curl_multi_perform(native_handle(), &count)) {
		//	return std::nullopt;
		// }

		assert(count >= 0);
		return static_cast<unsigned>(count);
	}

	void enable_multiplexing() noexcept {
		curl_multi_setopt(native_handle(), CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX);
	}

	bool poll(std::chrono::milliseconds timeout = std::chrono::seconds{1}) noexcept {
		return CURLM_OK == curl_multi_poll(native_handle(), nullptr, 0, static_cast<int>(timeout.count()), nullptr);
	}

	easycurl download(std::string_view url);
};

/*
auto process(multicurl & mhandle, easycurl & shandle) {
	struct curl_awaitable {
		multicurl & mhandle;
		easycurl & shandle;

		bool await_ready() const noexcept {
			return false;
		}

		void await_suspend(std::coroutine_handle<> h) {
			// add provided handle to multicurl and wake this coroutine when
			mhandle.add_handle(shandle);
		}

		void await_resume() const noexcept { }
	};

	return curl_awaitable(mhandle, shandle);
}
*/

} // namespace toolbox

#endif
