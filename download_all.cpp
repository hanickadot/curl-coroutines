#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <multicurl/task.hpp>
#include <numeric>
#include <utility/format.hpp>

auto all_done(std::vector<toolbox::fetch_task<std::vector<std::byte>>> in) -> toolbox::task<std::vector<std::vector<std::byte>>> {
	auto result = std::vector<std::vector<std::byte>>{};

	result.reserve(in.size());

	// I can't use algorithms :(

	for (auto & task: in) {
		result.emplace_back(co_await task);
	}

	co_return result;
}

auto download_files(std::vector<std::string> in) -> std::vector<toolbox::fetch_task<std::vector<std::byte>>> {
	auto result = std::vector<toolbox::fetch_task<std::vector<std::byte>>>{};

	std::transform(in.begin(), in.end(), std::back_inserter(result), [](const std::string & url) { return toolbox::binary_fetch(url); });

	return result;
}

auto process() -> toolbox::task<void> {
	// size_t total = co_await download_all();
	// std::cout << "Downloaded " << toolbox::data_size(total) << " total.\n";
	//
	const auto contents = co_await all_done(download_files({"https://hanicka.net/FPL04038.jpg", "https://hanicka.net/FPL07040.jpg", "https://hanicka.net/FPL04699.jpg"}));

	const auto total2 = std::accumulate(contents.begin(), contents.end(), size_t{0}, [](size_t lhs, const auto & container) { return lhs + container.size(); });
	std::cout << "list was " << toolbox::data_size(total2) << "\n";
}

int main() {
	try {
		process().get();
	} catch (...) {
		std::cout << "mooo\n";
	}
}
