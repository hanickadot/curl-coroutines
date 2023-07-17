#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <utility/format.hpp>

auto my_fetch(std::string url) -> toolbox::fetch_task<size_t> {
	toolbox::easycurl h{};

	h.url(url);
	h.write_function([](std::span<const std::byte>) {});

	co_await toolbox::with_multicurl(h);

	const auto size = h.get_content_length();

	if (!size.has_value()) {
		co_return 0;
	}

	co_return *size;
}

auto download_all() -> toolbox::task<size_t> {
	auto a = my_fetch("https://hanicka.net/FPL04859.jpg");
	auto b = my_fetch("https://www.google.com/");
	co_return (co_await a) + (co_await b);
}

int main() {
	size_t result = download_all();
	std::cout << toolbox::data_size(result) << "\n";
}