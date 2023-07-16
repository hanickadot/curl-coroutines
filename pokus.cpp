#include <iostream>
#include <multicurl/multicurl.hpp>
#include <utility/format.hpp>

int main() {
	toolbox::multicurl multi{};

	multi.enable_multiplexing();

	toolbox::easycurl handler{};

	size_t incoming_size = 0u;

	auto incoming_data = [&](std::span<const std::byte> in) {
		std::cout << "#1: incoming " << toolbox::data_size(in.size()) << "\n";
		incoming_size += in.size();
		// std::cout << "incoming: " << toolbox::data_size(in.size()) << " (total = " << toolbox::data_size(incoming_size) << ")\n";
	};

	auto headers = [](std::string_view in) {
		std::cout << "header: " << in;
	};

	handler.buffer_size(1024 * 1024);
	// handler.header_function(headers);
	// handler.debug_function(toolbox::custom_trace_function);
	// handler.write_function([&](std::span<const std::byte> in) {});
	handler.write_function(incoming_data);
	handler.url("https://www.google.com/");
	// handler.force_fresh();
	handler.pipewait();

	multi.add_handle(handler);

	// ----

	toolbox::easycurl handler2{};
	handler2.buffer_size(1024 * 1024);
	// handler2.header_function(headers);
	// handler2.debug_function(toolbox::custom_trace_function);
	handler2.write_function([](std::span<const std::byte> in) { std::cout << "#2: incoming " << toolbox::data_size(in.size()) << "\n"; });
	// handler2.write_function(incoming_data);
	handler2.url("https://www.google.com/");
	// handler2.force_fresh();
	handler2.pipewait();

	multi.add_handle(handler2);

	// ---

	const auto start = std::chrono::high_resolution_clock::now();

	for (;;) {
		const auto opt_count = multi.perform();

		for (;;) {
			int msgq = 0;
			struct CURLMsg * m = curl_multi_info_read(multi.native_handle(), &msgq);

			if (m && m->msg == CURLMSG_DONE) {
				if (m->easy_handle == handler.native_handle()) {
					const auto content_length = handler.get_content_length();

					if (content_length) {
						std::cout << "content-length: " << *content_length << "\n";
					} else {
						std::cout << "content-length: --\n";
					}
				}
				std::cout << msgq << " done...\n";
				break;
			}

			if (!m) {
				break;
			}
		}

		if (!opt_count || !*opt_count) {
			break;
		}

		if (!multi.poll()) {
			std::cout << "poll failed\n";
			break;
		}
	}

	const auto end = std::chrono::high_resolution_clock::now();

	std::cout << "downloaded " << toolbox::data_size(incoming_size) << " bytes in " << toolbox::duration(end - start) << "\n";
}