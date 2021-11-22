/*
 * task.h
 *
 *  Created on: Oct 29, 2021
 *      Author: bach
 */

#ifndef TASK_H_
#define TASK_H_

#include "libcurl/curl_wrapper.h"
#include "thread_pool.h"
#include "thread_pool2.h"

int fetch_audio(std::string_view url, std::string_view filename) {
	EasyCurly curl;
	CURLcode res = curl.perform(url, filename);
	return res;
}

// check file existence seems a bit complicated
// see https://stackoverflow.com/questions/12774207/fastest-way-to-check-if-a-file-exist-using-standard-c-c11-14-17-c#comment17266063_12774207
// some are cross-platform some perform better
#include <sys/stat.h>

inline bool file_exist(const std::string &name) {
	struct stat buffer;
	return (stat(name.c_str(), &buffer) == 0);
}

int fetch_html(std::string_view url) {

	EasyCurly curl;
	CURLcode res = curl.perform(url, "");

	std::cout << '[' << curl_easy_strerror(res) << ']' << '\n';

	std::cout << "--- document size " << curl.document().size() << "\n";
	//std::cout << "--- debug size " << curl.debug().size() << " captured ---\n"
	//          << curl.debug() << "---------\n";

	if (res == CURLE_OK) {

		const std::regex regex("podcastlink.*=\"(.*/(.*mp3))");
		std::smatch match;

		if (std::regex_search(curl.document(), match, regex)) {

			std::string url = match[1].str();
			std::string filename = match[2].str();

			std::cout << url << " .. " << filename << '\n';

			// it's a duplicate
			if (file_exist(filename))
				return res;

#if (1)
			fixed_thread_pool pool(1);
			pool.execute([=]() {
				fetch_audio(url, filename);
			});
#else
    	    thread_pool pool(1);
    	    pool.init();

    	    auto future = pool.submit(fetch_audio, pieces_match[1].str(), pieces_match[2].str());
    	    future.get();
#endif

		}
	}
	return res;
}

#endif /* TASK_H_ */
