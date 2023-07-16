#include <multicurl/fetch.hpp>
#include <multicurl/thread.hpp>
#include <utility/format.hpp>

auto process_in_paralel(int i) -> toolbox::different_thread_task<size_t> {
	for (int n = 0; n != 50; ++n) {
		std::this_thread::sleep_for(std::chrono::milliseconds{250});
		std::cout << "thread #" << i << " (" << std::this_thread::get_id() << "): " << n << "\n";
		co_await toolbox::switch_to_other_thread();
	}

	co_return 42;
}

auto download_all() -> toolbox::immediate_task<size_t> {
	auto a = process_in_paralel(1);
	auto b = process_in_paralel(2);
	auto c = process_in_paralel(3);

	std::this_thread::sleep_for(std::chrono::milliseconds{25000});

	co_await a;
	co_await b;
	co_await c;

	// auto index = curl.fetch_text("https://hanicka.net/");
	// auto image = curl.fetch_binary("https://hanicka.net/FPL04859.jpg");
	//
	// const size_t total_download = co_await result.size() + co_await image.size();
	//
	// std::cout << "total " << total_download << " bytes downloaded\n";

	co_return 11;
}

int main() {
	auto th = download_all();
	std::cout << th << "\n";
}