#include "multicurl.hpp"
#include <curl/curl.h>

toolbox::easycurl::easycurl(): handle{curl_easy_init()} { }

toolbox::easycurl::~easycurl() {
	if (handle) {
		curl_easy_cleanup(static_cast<CURL *>(handle));
	}
}

toolbox::multicurl::multicurl(): handle{curl_multi_init()} { }

toolbox::multicurl::~multicurl() {
	if (handle) {
		curl_multi_cleanup(static_cast<CURLM *>(handle));
	}
}
