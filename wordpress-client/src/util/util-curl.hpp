#pragma once
#include <cinttypes>
#include <cstring>
#include <functional>
#include <map>
#include <string>
#include <vector>

extern "C" {
#include <curl/curl.h>
}

namespace wpclient::util {
    typedef std::function<size_t(void*, size_t, size_t)>                   curl_io_callback_t;
    typedef std::function<int32_t(uint64_t, uint64_t, uint64_t, uint64_t)> curl_xferinfo_callback_t;
    typedef std::function<void(CURL*, curl_infotype, char*, size_t)>       curl_debug_callback_t;

    class curl {
        CURL*                              _curl;
        curl_io_callback_t                 _read_callback;
        curl_io_callback_t                 _write_callback;
        curl_xferinfo_callback_t           _xferinfo_callback;
        curl_debug_callback_t              _debug_callback;
        std::map<std::string, std::string> _headers;

        static int32_t debug_helper(CURL* handle, curl_infotype type, char* data, size_t size, wpclient::util::curl* self);
        static size_t  read_helper(void*, size_t, size_t, wpclient::util::curl*);
        static size_t  write_helper(void*, size_t, size_t, wpclient::util::curl*);
        static int32_t xferinfo_callback(wpclient::util::curl*, curl_off_t, curl_off_t, curl_off_t, curl_off_t);

    public:
        curl();
        ~curl();

        template<typename T>
        CURLcode set_option(CURLoption opt, T value)
        {
            return curl_easy_setopt(_curl, opt, value);
        }

        CURLcode set_option(CURLoption opt, bool value)
        {
            return curl_easy_setopt(_curl, opt, value ? 1L : 0L);
        }

        CURLcode set_option(CURLoption opt, const std::string& value)
        {
            return curl_easy_setopt(_curl, opt, value.c_str());
        }

        CURLcode set_option(CURLoption opt, std::string_view value)
        {
            return curl_easy_setopt(_curl, opt, value.data());
        }

        template<typename T>
        CURLcode get_info(CURLINFO info, T& value)
        {
            return curl_easy_getinfo(_curl, info, &value);
        }

        CURLcode get_info(CURLINFO info, std::string& value)
        {
            char* buf = nullptr;
            if (CURLcode res = curl_easy_getinfo(_curl, info, &buf); res != CURLE_OK)
                return res;
            if (buf)
                value = buf;
            return CURLE_OK;
        }

        void    clear_headers();
        void    clear_header(std::string_view header);
        void    set_header(std::string header, std::string value);
        CURLcode perform();
        void    reset();

        CURLcode set_read_callback(curl_io_callback_t cb);
        CURLcode set_write_callback(curl_io_callback_t cb);
        CURLcode set_xferinfo_callback(curl_xferinfo_callback_t cb);
        CURLcode set_debug_callback(curl_debug_callback_t cb);
    };
} // namespace wpclient::util
