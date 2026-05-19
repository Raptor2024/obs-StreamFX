#pragma once
#include "wp-models.hpp"

#include <functional>
#include <string>
#include <vector>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>

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

    class WpClient : public QObject {
        Q_OBJECT

    public:
        explicit WpClient(WpSite site, QObject* parent = nullptr);

        void testConnection(ResultCallback<std::string> callback);
        void fetchPosts(const std::string& status, int page, PostsCallback callback);
        void fetchPost(int64_t id, PostCallback callback);
        void createPost(const WpPost& post, PostCallback callback);
        void updatePost(const WpPost& post, PostCallback callback);
        void trashPost(int64_t id, VoidCallback callback);
        void fetchMedia(int page, MediasCallback callback);
        void uploadMedia(const std::string& file_path, MediaCallback callback);
        void fetchCategories(CatsCallback callback);
        void fetchTags(TagsCallback callback);

    signals:
        void networkError(QString message);

    private:
        WpSite                _site;
        QByteArray            _auth_header; // "Basic <base64>"
        QNetworkAccessManager _nam;

        QString        base_url() const;
        QNetworkRequest make_request(const QString& path) const;

        // Calls fn(http_status, body) when the reply finishes; handles transport errors.
        using ReplyHandler = std::function<void(int status, const QByteArray& body)>;
        void handle(QNetworkReply* reply, ReplyHandler fn);

        static ApiError parse_wp_error(int status, const QByteArray& body);
    };

} // namespace wpclient
