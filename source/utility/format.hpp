#ifndef UTILITY_UTILITY_HPP
#define UTILITY_UTILITY_HPP

#include <array>
#include <ostream>
#include <span>
#include <string_view>
#include <cassert>
#include <charconv>

namespace toolbox {

struct to_chars_result {
	std::string_view out;
	std::errc ec;

	constexpr operator bool() const noexcept {
		return ec == std::errc{};
	}
};

template <typename T, typename... Args> constexpr auto to_chars(std::span<char> output, T val, Args... args) {
	const auto r = std::to_chars(output.data(), output.data() + output.size(), val, args...);
	return to_chars_result{std::string_view(output.data(), r.ptr), r.ec};
}

struct duration {
	using representation_type = std::chrono::nanoseconds;
	representation_type value;

	template <typename Rep, typename Period> explicit(false) constexpr duration(std::chrono::duration<Rep, Period> n) noexcept: value{std::chrono::duration_cast<representation_type>(n)} { }

	friend auto operator<<(std::ostream & os, duration in) -> std::ostream & {
		// TODO do more
		return os << std::chrono::duration_cast<std::chrono::milliseconds>(in.value).count() << " ms";
	}
};

struct data_size {
	size_t amount{0};

	explicit(false) constexpr data_size(std::same_as<size_t> auto n) noexcept: amount{n} { }

	static constexpr auto add_to_buffer(std::span<char> buffer, std::span<const char> used, std::string_view text) noexcept -> std::span<const char> {
		assert(used.data() == buffer.data());
		assert(used.size() <= buffer.size());

		const auto remaining = buffer.subspan(used.size());

		if (remaining.size() < text.size()) {
			return {};
		}

		std::copy(text.begin(), text.end(), remaining.begin());

		return std::span<const char>(buffer).first(used.size() + text.size());
	}

	constexpr auto write_into(std::span<char> buffer) const noexcept -> std::span<const char> {
		if (amount < 1024u) {
			const auto r = to_chars(buffer, amount);
			return add_to_buffer(buffer, r.out, " Bi");
		} else if (amount < (1024u * 1024u)) {
			const auto r = to_chars(buffer, static_cast<double>(amount) / 1024u, std::chars_format::fixed, 2);
			return add_to_buffer(buffer, r.out, " kBi");
		} else if (amount < (1024u * 1024u * 1024u)) {
			const auto r = to_chars(buffer, static_cast<double>(amount) / (1024u * 1024u), std::chars_format::fixed, 2);
			return add_to_buffer(buffer, r.out, " MBi");
		} else {
			const auto r = to_chars(buffer, static_cast<double>(amount) / (1024u * 1024u * 1024u), std::chars_format::fixed, 2);
			return add_to_buffer(buffer, r.out, " GBi");
		}
	}

	friend auto operator<<(std::ostream & os, data_size in) -> std::ostream & {
		std::array<char, 32> buffer{};
		return os << std::string_view(in.write_into(buffer));
	}
};

} // namespace toolbox

#endif
