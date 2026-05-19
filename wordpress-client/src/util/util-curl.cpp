#include "util-curl.hpp"
#include <sstream>

int32_t wpclient::util::curl::debug_helper(CURL* handle, curl_infotype type, char* data, size_t size, wpclient::util::curl* self)
{
    if (self->_debug_callback)
        self->_debug_callback(handle, type, data, size);
    return 0;
}

size_t wpclient::util::curl::read_helper(void* ptr, size_t size, size_t count, wpclient::util::curl* self)
{
    if (self->_read_callback)
        return self->_read_callback(ptr, size, count);
    return size * count;
}

size_t wpclient::util::curl::write_helper(void* ptr, size_t size, size_t count, wpclient::util::curl* self)
{
    if (self->_write_callback)
        return self->_write_callback(ptr, size, count);
    return size * count;
}

int32_t wpclient::util::curl::xferinfo_callback(wpclient::util::curl* self, curl_off_t dlt, curl_off_t dln, curl_off_t ult, curl_off_t uln)
{
    if (self->_xferinfo_callback)
        return self->_xferinfo_callback(static_cast<uint64_t>(dlt), static_cast<uint64_t>(dln),
                                        static_cast<uint64_t>(ult), static_cast<uint64_t>(uln));
    return 0;
}

wpclient::util::curl::curl() : _curl(nullptr), _read_callback(), _write_callback(), _xferinfo_callback(), _debug_callback(), _headers()
{
    _curl = curl_easy_init();
    set_read_callback(nullptr);
    set_write_callback(nullptr);
    set_xferinfo_callback(nullptr);
    set_debug_callback(nullptr);

    set_option(CURLOPT_NOPROGRESS, false);
    set_option(CURLOPT_PATH_AS_IS, false);
    set_option(CURLOPT_VERBOSE, false);
    // Security: enforce TLS certificate verification
    set_option(CURLOPT_SSL_VERIFYPEER, 1L);
    set_option(CURLOPT_SSL_VERIFYHOST, 2L);
}

wpclient::util::curl::~curl()
{
    if (_curl)
        curl_easy_cleanup(_curl);
}

void wpclient::util::curl::clear_headers()
{
    _headers.clear();
}

void wpclient::util::curl::clear_header(std::string_view header)
{
    _headers.erase(std::string(header));
}

void wpclient::util::curl::set_header(std::string header, std::string value)
{
    _headers.insert_or_assign(std::move(header), std::move(value));
}

CURLcode wpclient::util::curl::perform()
{
    std::vector<char>  buffer;
    struct curl_slist* headers = nullptr;

    if (!_headers.empty()) {
        size_t total = 0;
        for (const auto& kv : _headers)
            total += kv.first.size() + 2 + kv.second.size() + 1;
        buffer.resize(total * 2);

        size_t offset = 0;
        for (const auto& kv : _headers) {
            size_t sz = kv.first.size() + 2 + kv.second.size() + 1;
            snprintf(buffer.data() + offset, sz, "%s: %s", kv.first.c_str(), kv.second.c_str());
            headers = curl_slist_append(headers, buffer.data() + offset);
            offset += sz;
        }
        set_option<struct curl_slist*>(CURLOPT_HTTPHEADER, headers);
    }

    CURLcode res = curl_easy_perform(_curl);

    if (headers) {
        set_option<struct curl_slist*>(CURLOPT_HTTPHEADER, nullptr);
        curl_slist_free_all(headers);
    }

    return res;
}

void wpclient::util::curl::reset()
{
    curl_easy_reset(_curl);
}

CURLcode wpclient::util::curl::set_read_callback(curl_io_callback_t cb)
{
    _read_callback = std::move(cb);
    if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_READDATA, this); res != CURLE_OK)
        return res;
    return curl_easy_setopt(_curl, CURLOPT_READFUNCTION, &read_helper);
}

CURLcode wpclient::util::curl::set_write_callback(curl_io_callback_t cb)
{
    _write_callback = std::move(cb);
    if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_WRITEDATA, this); res != CURLE_OK)
        return res;
    return curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, &write_helper);
}

CURLcode wpclient::util::curl::set_xferinfo_callback(curl_xferinfo_callback_t cb)
{
    _xferinfo_callback = std::move(cb);
    if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_XFERINFODATA, this); res != CURLE_OK)
        return res;
    return curl_easy_setopt(_curl, CURLOPT_XFERINFOFUNCTION, &xferinfo_callback);
}

CURLcode wpclient::util::curl::set_debug_callback(curl_debug_callback_t cb)
{
    _debug_callback = std::move(cb);
    if (CURLcode res = curl_easy_setopt(_curl, CURLOPT_DEBUGDATA, this); res != CURLE_OK)
        return res;
    return curl_easy_setopt(_curl, CURLOPT_DEBUGFUNCTION, &debug_helper);
}
