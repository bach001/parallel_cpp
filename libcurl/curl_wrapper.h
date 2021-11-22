/*
 * rip_audio.h
 *
 *  Created on: Oct 28, 2021
 *      Author: bach
 */

#ifndef LIBCURL_CURL_WRAPPER_H_
#define LIBCURL_CURL_WRAPPER_H_

#include "../sys_lib.h"
#include "curl/curl.h"

// A simple wrapper class for the C functions in libcurl
class EasyCurly {
public:
    // default constructor
    EasyCurly() : handle(curl_easy_init()) {
        if(handle == nullptr) throw std::runtime_error("curl_easy_init failed");

        // Set "this" as data pointer in callbacks to be able to make a call to the
        // correct EasyCurly object. There are a lot more callback functions you
        // could add here if you need them.
        setopt(CURLOPT_WRITEDATA, this);
        setopt(CURLOPT_DEBUGDATA, this);
        setopt(CURLOPT_XFERINFODATA, this);

        // Setup of proxy/callback functions. There should be one for each function
        // above.
        setopt(CURLOPT_WRITEFUNCTION, write_callback);
        setopt(CURLOPT_DEBUGFUNCTION, debug_callback);
        setopt(CURLOPT_XFERINFOFUNCTION, progress_callback);

        // some default options, remove those you usually don't want
        setopt(CURLOPT_NOPROGRESS, 0);       // turn on progress callbacks
        setopt(CURLOPT_FOLLOWLOCATION, 1L);  // redirects
        setopt(CURLOPT_HTTPPROXYTUNNEL, 1L); // corp. proxies etc.
        // setopt(CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        setopt(CURLOPT_VERBOSE, 1L); // we want it all
    }

    // copy constructor
    EasyCurly(const EasyCurly& other) : handle(curl_easy_duphandle(other.handle)) {
        if(handle == nullptr) throw std::runtime_error("curl_easy_duphandle failed");
        // State information is not shared when using curl_easy_duphandle. Only the
        // options you've set (so you can create one CURL object, set its options and
        // then use as a template for other objects. The document and debug data are
        // therefor also not copied.
    }

    // move constructor
    EasyCurly(EasyCurly&& other) :
        handle(std::exchange(other.handle, nullptr)),
        m_document(std::move(other.m_document)), m_debug(std::move(other.m_debug)) {}

    // copy assignment
    EasyCurly& operator=(const EasyCurly& other) {
        CURL* tmp_handle = curl_easy_duphandle(other.handle);
        if(handle == nullptr) throw std::runtime_error("curl_easy_duphandle failed");
        // dup succeeded, now destroy any handle we might have and copy the tmp
        curl_easy_cleanup(handle);
        handle = tmp_handle;
        return *this;
    }

    // move assignment
    EasyCurly& operator=(EasyCurly&& other) {
        std::swap(handle, other.handle);
        std::swap(m_document, other.m_document);
        std::swap(m_debug, other.m_debug);
        return *this;
    }

    virtual ~EasyCurly() { curl_easy_cleanup(handle); }

    // To be able to use an instance of EasyCurly with C interfaces if you don't add
    // a function to this class for it, this operator will help
    operator CURL*() { return handle; }

    // generic curl_easy_setopt wrapper
    template<typename T>
    CURLcode setopt(CURLoption option, T v) {
        return curl_easy_setopt(handle, option, v);
    }

    // perform by supplying url
    CURLcode perform(std::string_view url, std::string_view _filename) {
    	filename = _filename;
        setopt(CURLOPT_URL, url.data());
        return perform();
    }

    // perform with a previously supplied url
    CURLcode perform() {
        m_document.clear();
        m_debug.clear();
        return curl_easy_perform(handle);
    }

    bool file_name() {
    	return filename != "";
    }

    // get collected data
    std::string const& document() const { return m_document; }
    std::string const& debug() const { return m_debug; }

/*
    size_t write_to_file(void *contents, size_t size, size_t nmemb, void *userp) {
    	size_t realsize = size * nmemb;
    	auto file = reinterpret_cast<std::ofstream*>(userp);
    	file->write(reinterpret_cast<const char*>(contents), realsize);
    	return realsize;
    }
*/

    //void save_to_file(CURL *curl, std::string &filename) {
    //	static std::ofstream file(filename, std::ios::binary);
    //	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
    //	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(&file));
    //}

    // callbacks from proxy functions
    virtual size_t on_write(char* ptr, size_t total_size) {
        m_document.insert(m_document.end(), ptr, ptr + total_size);

        // std::cout << "<write_callback> size " << total_size << "\n";
        return total_size;
    }

    virtual size_t on_write1(char* ptr, size_t total_size) {
        //m_document.insert(m_document.end(), ptr, ptr + total_size);
    	//std::ofstream file(filename, std::ios::binary | std::ios_base::app);
    	std::ofstream out(filename, std::ios_base::out | std::ios_base::app);
    	out.write(ptr, total_size);
    	out.close();
        // std::cout << "<write_callback> size " << total_size << "\n";
        return total_size;
    }

    virtual int on_debug(curl_infotype /*type*/, char* data, size_t size) {
        m_debug.insert(m_debug.end(), data, data + size);
        // std::cout << "<debug>\n";
        return 0; // must return 0
    }
    // override this to make a progress bar ...
    virtual int on_progress(curl_off_t /*dltotal*/, curl_off_t /*dlnow*/,
                            curl_off_t /*ultotal*/, curl_off_t /*ulnow*/) {
        // std::cout << "<progress>\n";
        return 0;
    }

private:
    // a private class to initialize and cleanup once
    class CurlGlobalInit {
    public:
        CurlGlobalInit() {
            // std::cout << "CurlGlobalInit\n";
            CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
            if(res) throw std::runtime_error("curl_global_init failed");
        }
        ~CurlGlobalInit() {
            // std::cout << "~CurlGlobalInit\n";
            curl_global_cleanup();
        }
    };

    // callback functions - has to be static to work with the C interface in curl
    // use the data pointer (this) that we set in the constructor and cast it back
    // to a EasyCurly* and call the event handler in the correct object.
    static size_t write_callback(char* ptr, size_t size, size_t nmemb,
                                 void* userdata) {
        EasyCurly* ecurly = static_cast<EasyCurly*>(userdata);
        if (ecurly->file_name()) {
        	return ecurly->on_write1(ptr, nmemb * size);
        }
        return ecurly->on_write(ptr, nmemb * size); // size==1 really
    }
    static int debug_callback(CURL* /*handle*/, curl_infotype type, char* data,
                              size_t size, void* userptr) {
        EasyCurly* ecurly = static_cast<EasyCurly*>(userptr);
        return ecurly->on_debug(type, data, size);
    }
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow,
                                 curl_off_t ultotal, curl_off_t ulnow) {
        EasyCurly* ecurly = static_cast<EasyCurly*>(clientp);
        return ecurly->on_progress(dltotal, dlnow, ultotal, ulnow);
    }

    // resources
    CURL* handle;
    std::string m_document{};
    std::string m_debug{};
    std::string filename{};

    // a static initializer object
    static CurlGlobalInit setup_and_teardown;
};

// end of wrapper class
// --------------------------------------------------------------------------


#if 0

#include <curl/curl.h>
#include <string>
#include <memory>
#include <functional>
#include <exception>
#include <iostream>

using eash_handle = std::unique_ptr<CURL, std::function<void(CURL*)>>;

eash_handle create_ease_handle() {
	auto curl = eash_handle(curl_easy_init(), curl_easy_cleanup);
	if (!curl) {
		throw std::runtime_error("Failed creating CURL easy object");
	}
	return curl;
}

size_t write_to_file(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	auto file = reinterpret_cast<std::ofstream*>(userp);
	file->write(reinterpret_cast<const char*>(contents), realsize);
	return realsize;
}

void save_to_file(CURL *curl, std::string &filename) {
	static std::ofstream file(filename, std::ios::binary);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_to_file);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, reinterpret_cast<void*>(&file));
}

std::string fetch_audio(const std::string &url, const std::string &filename) {
	//std::list<easyhandle> handles(3);

	/* init easy stacks
	 try
	 {
	 std::for_each(handles.begin(), handles.end(), [](auto& handle) {handle = CreateEasyHandle(); });
	 }
	 catch (const std::exception& ex)
	 {
	 std::cerr << ex.what() << std::endl;
	 return -1;
	 }
	 */
	eash_handle handle = create_ease_handle();
	//for (auto& handle : handles)
	/* set options */
	curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());
	//set_ssl(handle.get());
	save_to_file(handle.get(), filename);
	curl_easy_setopt(handle.get(), CURLOPT_USERAGENT, "libcurl-agent/1.0");
	/* Perform the request, res will get the return code */
	auto res = curl_easy_perform(handle.get());
	/* Check for errors */
	if (res != CURLE_OK) {
		std::cerr << "curl_easy_perform() failed:" << curl_easy_strerror(res)
				<< std::endl;
		return -1;
	}

	return 0;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

static size_t mem_write(void *contents, size_t size, size_t nmemb,
		void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct*) userp;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (!ptr) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

int fetch_audio(const std::string &url, const std::string &filename) {

	struct MemoryStruct chunk;

	chunk.memory = malloc(1); /* will be grown as needed by the realloc above */
	chunk.size = 0; /* no data at this point */

	eash_handle handle = create_ease_handle();

	/* specify URL to get */
	curl_easy_setopt(handle.get(), CURLOPT_URL, url.c_str());

	/* send all data to this function  */
	curl_easy_setopt(handle.get(), CURLOPT_WRITEFUNCTION, mem_write);

	/* we pass our 'chunk' struct to the callback function */
	curl_easy_setopt(handle.get(), CURLOPT_WRITEDATA, (void* )&chunk);

	/* some servers don't like requests that are made without a user-agent
	 field, so we provide one */
	curl_easy_setopt(handle.get(), CURLOPT_USERAGENT, "libcurl-agent/1.0");

	/* get it! */
	auto res = curl_easy_perform(handle.get());

	/* check for errors */
	if (res != CURLE_OK) {
		std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << '\n';
	} else {
		/*
		 * Now, our chunk.memory points to a memory block that is chunk.size
		 * bytes big and contains the remote file.
		 *
		 * Do something nice with it!
		 */



		//printf("%lu bytes retrieved\n", (unsigned long) chunk.size);
	}

	free(chunk.memory);

	return 0;
}
#endif

#endif /* LIBCURL_CURL_WRAPPER_H_ */
