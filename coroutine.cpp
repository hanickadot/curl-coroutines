#include <multicurl/coroutine.hpp>
#include <utility/format.hpp>

auto download_all() -> toolbox::multicurl_download_task<void> {
	auto index_promise = toolbox::fetch_text("https://hanicka.net");
	auto image_promise = toolbox::fetch_binary("https://hanicka.net/FPL07048.jpg");

	// co_await is needed as this needs to talk with promise of current coroutine
	co_await index_promise.start();
	co_await index_promise.start();

	// this will "block" until it's really downloaded
	auto index = co_await index_promise;
	auto image = co_await image_promise;

	// or ...
	// co_await all_downloads_to_finish();

	std::cout << index.size() + image.size() << " bytes downloaded\n";
}

int main() {
	download_all();
}
