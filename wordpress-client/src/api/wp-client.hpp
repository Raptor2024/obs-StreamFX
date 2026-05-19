#pragma once
#include "wp-models.hpp"

#include <functional>
#include <string>
#include <vector>

#include <QObject>
#include <QThreadPool>

namespace wpclient {

    struct ApiError {
        int         http_status = 0;
        std::string code;
        std::string message;
    };

    template<typename T>
    using ResultCallback = std::function<void(bool ok, T result, ApiError err)>;

    using VoidCallback   = std::function<void(bool ok, ApiError err)>;
    using PostCallback   = ResultCallback<WpPost>;
    using PostsCallback  = ResultCallback<std::vector<WpPost>>;
    using MediaCallback  = ResultCallback<WpMedia>;
    using MediasCallback = ResultCallback<std::vector<WpMedia>>;
    using CatsCallback   = ResultCallback<std::vector<WpCategory>>;
    using TagsCallback   = ResultCallback<std::vector<WpTag>>;

    // All public methods dispatch work onto a thread pool and invoke the callback
    // on the calling thread via Qt's queued connection mechanism.
    class WpClient : public QObject {
        Q_OBJECT

    public:
        explicit WpClient(WpSite site, QObject* parent = nullptr);

        // Verify credentials — calls /wp-json/wp/v2/users/me
        void testConnection(ResultCallback<std::string> callback);

        // Posts
        void fetchPosts(const std::string& status, int page, PostsCallback callback);
        void fetchPost(int64_t id, PostCallback callback);
        void createPost(const WpPost& post, PostCallback callback);
        void updatePost(const WpPost& post, PostCallback callback);
        void trashPost(int64_t id, VoidCallback callback);

        // Media
        void fetchMedia(int page, MediasCallback callback);
        void uploadMedia(const std::string& file_path, MediaCallback callback);

        // Taxonomy
        void fetchCategories(CatsCallback callback);
        void fetchTags(TagsCallback callback);

    signals:
        void networkError(QString message);

    private:
        WpSite      _site;
        std::string _auth_header; // "Basic <base64>"

        std::string base_url() const;

        // Low-level helpers (run on worker thread)
        struct Response {
            int         status = 0;
            std::string body;
            std::string curl_error;
        };

        Response get(const std::string& path);
        Response post_json(const std::string& path, const std::string& json_body);
        Response put_json(const std::string& path, const std::string& json_body);
        Response delete_req(const std::string& path);
        Response post_multipart(const std::string& path, const std::string& file_path,
                                const std::string& mime_type, const std::string& filename);

        static ApiError parse_error(const Response& r);
    };

} // namespace wpclient
