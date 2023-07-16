#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <multicurl/task.hpp>
#include <utility/format.hpp>

template <typename T> concept byte_like_value_type = sizeof(typename T::value_type) == 1u;

template <byte_like_value_type Result> auto generic_fetch(std::string_view url) -> toolbox::fetch_task<Result> {
	toolbox::easycurl request{};

	request.pipewait();
	// request.force_fresh();
	request.url(url);
	// request.debug_function(toolbox::custom_trace_function);

	using value_type = typename Result::value_type;
	Result output{};

	// this will be called multiple times
	auto write_to_string = [&output](std::span<const std::byte> in) {
		auto strin = std::span<const value_type>(reinterpret_cast<const value_type *>(in.data()), in.size());
		std::copy(strin.begin(), strin.end(), std::back_inserter(output));
	};

	request.write_function(write_to_string);

	std::cout << "[starting downloading of '" << url << "'...]\n";

	// register request to multicurl and go back to original caller
	co_await toolbox::with_multicurl(request);

	std::cout << "[download of '" << url << "' finished... (size = " << toolbox::data_size(output.size()) << ", code = " << request.get_response_code() << ")]\n";

	if (request.get_response_code() != 200) {
		throw std::invalid_argument{"42"};
		co_return Result{};
	}

	co_return output;
}

auto fetch_binary(std::string_view url) {
	return generic_fetch<std::vector<std::byte>>(url);
}

auto fetch_string(std::string_view url) {
	return generic_fetch<std::string>(url);
}

auto fetch_size(std::string_view url) -> toolbox::task<size_t> {
	auto result = fetch_binary(url);
	co_return (co_await result).size();
}

auto download_all() -> toolbox::task<size_t> {
	try {
		auto index = co_await fetch_string("https://hanickadot.github.io/index2.html");

		if (index == "") {
			co_return 42;
		}
	} catch (...) {
		std::cout << "missing index2.html\n";
	}

	auto index = co_await fetch_binary("https://hanickadot.github.io/index.html");

	if (index == std::vector<std::byte>{}) {
		co_return 42;
	}

	auto image1 = fetch_binary("https://hanickadot.github.io/new-york-2022/img/FPL03986.jpg");
	auto image2 = fetch_binary("https://hanickadot.github.io/new-york-2022/img/FPL04038.jpg");

	// co_return co_await image2_size;

	// const size_t image_size = (co_await image).size();
	// const size_t index_size = (co_await index).size();

	co_return (co_await image1).size() + (co_await image2).size();
}

auto intermediate() -> toolbox::task<void> {
	size_t total = co_await download_all();
	std::cout << "downloaded " << toolbox::data_size(total) << "\n";
}

int main() {
	try {
		intermediate().get();
	} catch (...) {
		std::cout << "mooo\n";
	}
}