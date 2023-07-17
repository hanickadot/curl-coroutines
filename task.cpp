#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <numeric>
#include <utility/format.hpp>

auto download_all() -> toolbox::task<size_t> {
	try {
		auto index = co_await toolbox::string_fetch("https://hanickadot.github.io/index2.html");
		std::cout << "index2.html is " << toolbox::data_size(index.size()) << "\n";
	} catch (...) {
		std::cout << "Missing index2.html\n";
	}

	auto index = toolbox::binary_fetch("https://hanickadot.github.io/index.html");
	auto image1 = toolbox::binary_fetch("https://hanickadot.github.io/new-york-2022/img/FPL03986.jpg");
	auto image2 = toolbox::binary_fetch("https://hanickadot.github.io/new-york-2022/img/FPL04038.jpg");

	co_return (co_await index).size() + (co_await image1).size() + (co_await image2).size();
}

auto intermediate() -> toolbox::task<void> {
	size_t total = co_await download_all();
	std::cout << "Downloaded " << toolbox::data_size(total) << " total.\n";
}

int main() {
	intermediate();
}
