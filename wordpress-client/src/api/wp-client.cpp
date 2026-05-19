#include "wp-client.hpp"
#include "wp-auth.hpp"
#include "util/util-curl.hpp"

#include <fstream>
#include <sstream>

#include <QFileInfo>
#include <QMimeDatabase>
#include <QRunnable>
#include <QThreadPool>

extern "C" {
#include <curl/curl.h>
}

namespace wpclient {

// ---------------------------------------------------------------------------
// Helper: async task wrapper
// ---------------------------------------------------------------------------

template<typename F>
static void run_async(F&& fn)
{
    struct Task : public QRunnable {
        F fn;
        explicit Task(F&& f) : fn(std::move(f)) { setAutoDelete(true); }
        void run() override { fn(); }
    };
    QThreadPool::globalInstance()->start(new Task(std::move(fn)));
}

// ---------------------------------------------------------------------------
// WpClient
// ---------------------------------------------------------------------------

WpClient::WpClient(WpSite site, QObject* parent)
    : QObject(parent), _site(std::move(site))
{
    _auth_header = WpAuth::make_auth_header(_site.id);
}

std::string WpClient::base_url() const
{
    std::string url = _site.url;
    if (!url.empty() && url.back() == '/')
        url.pop_back();
    return url + "/wp-json/wp/v2";
}

// ---------------------------------------------------------------------------
// Low-level HTTP helpers (run on worker threads)
// ---------------------------------------------------------------------------

static std::string collect_write(wpclient::util::curl& c, std::string& body_out)
{
    c.set_write_callback([&body_out](void* data, size_t s1, size_t s2) -> size_t {
        size_t n = s1 * s2;
        body_out.append(static_cast<const char*>(data), n);
        return n;
    });
    return {};
}

WpClient::Response WpClient::get(const std::string& path)
{
    Response r;
    wpclient::util::curl c;
    collect_write(c, r.body);
    c.set_header("Authorization", _auth_header);
    c.set_header("Accept", "application/json");
    c.set_option(CURLOPT_URL, base_url() + path);
    c.set_option(CURLOPT_HTTPGET, 1L);
    c.set_option(CURLOPT_TIMEOUT, 30L);

    if (CURLcode res = c.perform(); res != CURLE_OK) {
        r.curl_error = curl_easy_strerror(res);
        return r;
    }
    long code = 0;
    c.get_info(CURLINFO_HTTP_CODE, code);
    r.status = static_cast<int>(code);
    return r;
}

WpClient::Response WpClient::post_json(const std::string& path, const std::string& json_body)
{
    Response r;
    wpclient::util::curl c;
    collect_write(c, r.body);
    c.set_header("Authorization", _auth_header);
    c.set_header("Content-Type", "application/json");
    c.set_header("Accept", "application/json");
    c.set_option(CURLOPT_URL, base_url() + path);
    c.set_option(CURLOPT_POST, 1L);
    c.set_option(CURLOPT_POSTFIELDS, json_body.c_str());
    c.set_option(CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    c.set_option(CURLOPT_TIMEOUT, 30L);

    if (CURLcode res = c.perform(); res != CURLE_OK) {
        r.curl_error = curl_easy_strerror(res);
        return r;
    }
    long code = 0;
    c.get_info(CURLINFO_HTTP_CODE, code);
    r.status = static_cast<int>(code);
    return r;
}

WpClient::Response WpClient::put_json(const std::string& path, const std::string& json_body)
{
    Response r;
    wpclient::util::curl c;
    collect_write(c, r.body);
    c.set_header("Authorization", _auth_header);
    c.set_header("Content-Type", "application/json");
    c.set_header("Accept", "application/json");
    c.set_option(CURLOPT_URL, base_url() + path);
    c.set_option(CURLOPT_CUSTOMREQUEST, "PUT");
    c.set_option(CURLOPT_POSTFIELDS, json_body.c_str());
    c.set_option(CURLOPT_POSTFIELDSIZE, static_cast<long>(json_body.size()));
    c.set_option(CURLOPT_TIMEOUT, 30L);

    if (CURLcode res = c.perform(); res != CURLE_OK) {
        r.curl_error = curl_easy_strerror(res);
        return r;
    }
    long code = 0;
    c.get_info(CURLINFO_HTTP_CODE, code);
    r.status = static_cast<int>(code);
    return r;
}

WpClient::Response WpClient::delete_req(const std::string& path)
{
    Response r;
    wpclient::util::curl c;
    collect_write(c, r.body);
    c.set_header("Authorization", _auth_header);
    c.set_header("Accept", "application/json");
    c.set_option(CURLOPT_URL, base_url() + path);
    c.set_option(CURLOPT_CUSTOMREQUEST, "DELETE");
    c.set_option(CURLOPT_TIMEOUT, 30L);

    if (CURLcode res = c.perform(); res != CURLE_OK) {
        r.curl_error = curl_easy_strerror(res);
        return r;
    }
    long code = 0;
    c.get_info(CURLINFO_HTTP_CODE, code);
    r.status = static_cast<int>(code);
    return r;
}

WpClient::Response WpClient::post_multipart(const std::string& path, const std::string& file_path,
                                             const std::string& mime_type, const std::string& filename)
{
    Response r;
    wpclient::util::curl c;
    collect_write(c, r.body);
    c.set_header("Authorization", _auth_header);
    c.set_header("Accept", "application/json");
    c.set_option(CURLOPT_URL, base_url() + path);
    c.set_option(CURLOPT_TIMEOUT, 120L);

    curl_mime* mime  = curl_mime_init(nullptr);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, file_path.c_str());
    curl_mime_type(part, mime_type.c_str());
    curl_mime_filename(part, filename.c_str());

    c.set_option(CURLOPT_MIMEPOST, mime);

    if (CURLcode res = c.perform(); res != CURLE_OK)
        r.curl_error = curl_easy_strerror(res);
    else {
        long code = 0;
        c.get_info(CURLINFO_HTTP_CODE, code);
        r.status = static_cast<int>(code);
    }
    curl_mime_free(mime);
    return r;
}

ApiError WpClient::parse_error(const Response& r)
{
    ApiError err;
    err.http_status = r.status;
    if (!r.curl_error.empty()) {
        err.message = r.curl_error;
        return err;
    }
    try {
        auto j = nlohmann::json::parse(r.body);
        err.code    = j.value("code",    std::string{});
        err.message = j.value("message", std::string{});
    } catch (...) {
        err.message = "HTTP " + std::to_string(r.status);
    }
    return err;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void WpClient::testConnection(ResultCallback<std::string> callback)
{
    run_async([this, cb = std::move(callback)]() {
        auto r = get("/users/me");
        if (!r.curl_error.empty() || r.status != 200) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            auto j    = nlohmann::json::parse(r.body);
            std::string name = j.value("name", std::string{});
            cb(true, name, {});
        } catch (...) {
            cb(false, {}, {0, "parse_error", "Failed to parse response"});
        }
    });
}

void WpClient::fetchPosts(const std::string& status, int page, PostsCallback callback)
{
    run_async([this, status, page, cb = std::move(callback)]() {
        std::string path = "/posts?per_page=50&page=" + std::to_string(page);
        if (!status.empty())
            path += "&status=" + status;

        auto r = get(path);
        if (!r.curl_error.empty() || (r.status != 200 && r.status != 206)) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            auto j     = nlohmann::json::parse(r.body);
            auto posts = j.get<std::vector<WpPost>>();
            cb(true, std::move(posts), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchPost(int64_t id, PostCallback callback)
{
    run_async([this, id, cb = std::move(callback)]() {
        auto r = get("/posts/" + std::to_string(id));
        if (!r.curl_error.empty() || r.status != 200) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<WpPost>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::createPost(const WpPost& post, PostCallback callback)
{
    nlohmann::json j = post;
    std::string body = j.dump();
    run_async([this, body, cb = std::move(callback)]() {
        auto r = post_json("/posts", body);
        if (!r.curl_error.empty() || (r.status != 200 && r.status != 201)) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<WpPost>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::updatePost(const WpPost& post, PostCallback callback)
{
    nlohmann::json j = post;
    std::string body = j.dump();
    int64_t id = post.id;
    run_async([this, id, body, cb = std::move(callback)]() {
        auto r = put_json("/posts/" + std::to_string(id), body);
        if (!r.curl_error.empty() || r.status != 200) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<WpPost>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::trashPost(int64_t id, VoidCallback callback)
{
    run_async([this, id, cb = std::move(callback)]() {
        auto r = delete_req("/posts/" + std::to_string(id));
        if (!r.curl_error.empty() || (r.status != 200 && r.status != 201)) {
            cb(false, parse_error(r));
        } else {
            cb(true, {});
        }
    });
}

void WpClient::fetchMedia(int page, MediasCallback callback)
{
    run_async([this, page, cb = std::move(callback)]() {
        auto r = get("/media?per_page=50&page=" + std::to_string(page));
        if (!r.curl_error.empty() || (r.status != 200 && r.status != 206)) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            auto items = nlohmann::json::parse(r.body).get<std::vector<WpMedia>>();
            cb(true, std::move(items), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::uploadMedia(const std::string& file_path, MediaCallback callback)
{
    run_async([this, file_path, cb = std::move(callback)]() {
        QFileInfo fi(QString::fromStdString(file_path));
        QMimeDatabase db;
        std::string mime = db.mimeTypeForFile(fi).name().toStdString();
        std::string name = fi.fileName().toStdString();

        auto r = post_multipart("/media", file_path, mime, name);
        if (!r.curl_error.empty() || (r.status != 200 && r.status != 201)) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<WpMedia>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchCategories(CatsCallback callback)
{
    run_async([this, cb = std::move(callback)]() {
        auto r = get("/categories?per_page=100");
        if (!r.curl_error.empty() || r.status != 200) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<std::vector<WpCategory>>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchTags(TagsCallback callback)
{
    run_async([this, cb = std::move(callback)]() {
        auto r = get("/tags?per_page=100");
        if (!r.curl_error.empty() || r.status != 200) {
            cb(false, {}, parse_error(r));
            return;
        }
        try {
            cb(true, nlohmann::json::parse(r.body).get<std::vector<WpTag>>(), {});
        } catch (const std::exception& ex) {
            cb(false, {}, {0, "parse_error", ex.what()});
        }
    });
}

} // namespace wpclient
