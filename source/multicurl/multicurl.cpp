#include "multicurl.hpp"
#include <curl/curl.h>

toolbox::easycurl::easycurl(): handle{curl_easy_init()} { }

toolbox::easycurl::~easycurl() {
	if (handle) {
		curl_easy_cleanup(static_cast<CURL *>(handle));
	}
}

void toolbox::easycurl::url(const char * value) noexcept {
	curl_easy_setopt(native_handle(), CURLOPT_URL, value);
}

void toolbox::easycurl::url(const std::string & value) noexcept {
	url(value.c_str());
}

void toolbox::easycurl::url(std::string_view value) {
	// TODO I know :(
	url(std::string(value));
}

void toolbox::easycurl::buffer_size(signed long sz) noexcept {
	curl_easy_setopt(native_handle(), CURLOPT_BUFFERSIZE, sz);
}

unsigned toolbox::easycurl::perform() noexcept {
	return static_cast<unsigned>(curl_easy_perform(native_handle()));
}

void toolbox::easycurl::force_fresh() noexcept {
	curl_easy_setopt(native_handle(), CURLOPT_FRESH_CONNECT, 1L);
}

void toolbox::easycurl::pipewait() {
	curl_easy_setopt(native_handle(), CURLOPT_PIPEWAIT, 1L);
}

void toolbox::easycurl::follow_location() {
	curl_easy_setopt(native_handle(), CURLOPT_FOLLOWLOCATION, 1L);
}

auto toolbox::easycurl::get_content_length() const noexcept -> std::optional<size_t> {
	curl_off_t result{0};
	curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &result);

	if (result == -1) {
		return std::nullopt;
	}

	return static_cast<size_t>(result);
}

long toolbox::easycurl::get_response_code() const noexcept {
	long result = 0;
	curl_easy_getinfo(handle, CURLINFO_RESPONSE_CODE, &result);
	return result;
}

toolbox::multicurl::multicurl(): handle{curl_multi_init()} { }

toolbox::multicurl::~multicurl() {
	if (handle) {
		curl_multi_cleanup(static_cast<CURLM *>(handle));
	}
}
