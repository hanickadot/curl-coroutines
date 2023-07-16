#include <iostream>
#include <multicurl/fetch-task.hpp>
#include <multicurl/task.hpp>
#include <utility/format.hpp>

auto fetch(std::string_view url) -> toolbox::fetch_task<std::string> {
	toolbox::easycurl request{};

	request.url(url);
	request.debug_function(toolbox::custom_trace_function);

	std::string output{};

	// this will be called multiple times
	// auto write_to_string = [&output](std::span<const std::byte> in) {
	//	output.reserve(output.size() + in.size());
	//	auto strin = std::string_view(reinterpret_cast<const char *>(in.data()), in.size());
	//	std::copy(strin.begin(), strin.end(), std::back_inserter(output));
	//};
	//
	// request.write_function(write_to_string);

	std::cout << "before jumping back...\n";
	// register request to multicurl and go back to original caller
	co_await toolbox::with_multicurl(request);
	std::cout << "after multicurl...\n";

	co_return output;
}

auto download_all() -> toolbox::immediate_task<size_t> {
	auto index = fetch("https://hanicka.net/");
	// auto image = fetch("https://hanicka.net/FPL07048.jpg");

	std::cout << "after fetch...\n";

	co_return (co_await index).size();
	// co_return (co_await index).size() + (co_await image).size();
}

int main() {
	size_t size = download_all();
	std::cout << "downloaded " << toolbox::data_size(size) << "\n";
}