#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <utility/format.hpp>

auto download_all() -> toolbox::task<std::vector<std::byte>> {
	auto a = toolbox::binary_fetch("https://hanicka.net/");
	auto b = toolbox::binary_fetch("https://www.google.com/");

	auto ra = co_await a;
	auto rb = co_await b;

	std::vector<std::byte> output;

	std::copy(ra.begin(), ra.end(), std::back_inserter(output));
	std::copy(rb.begin(), rb.end(), std::back_inserter(output));

	co_return output;
}

int main() {
	std::vector<std::byte> r = download_all();
	std::cout << r.size() << "\n";
}
