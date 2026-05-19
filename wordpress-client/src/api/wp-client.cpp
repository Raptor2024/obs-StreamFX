#include "wp-client.hpp"
#include "wp-auth.hpp"

#include <QFile>
#include <QFileInfo>
#include <QMimeDatabase>
#include <QSslConfiguration>
#include <QSslSocket>

namespace wpclient {

WpClient::WpClient(WpSite site, QObject* parent)
    : QObject(parent), _site(std::move(site))
{
    std::string token = WpAuth::make_auth_header(_site.id);
    _auth_header = QByteArray::fromStdString(token);
    std::fill(token.begin(), token.end(), '\0');

    // Security: enforce peer TLS certificate verification
    QSslConfiguration ssl = QSslConfiguration::defaultConfiguration();
    ssl.setPeerVerifyMode(QSslSocket::VerifyPeer);
    QSslConfiguration::setDefaultConfiguration(ssl);
}

QString WpClient::base_url() const
{
    QString url = QString::fromStdString(_site.url);
    if (url.endsWith('/'))
        url.chop(1);
    return url + "/wp-json/wp/v2";
}

QNetworkRequest WpClient::make_request(const QString& path) const
{
    QNetworkRequest req(QUrl(base_url() + path));
    req.setRawHeader("Authorization", _auth_header);
    req.setRawHeader("Accept", "application/json");
    req.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                     QNetworkRequest::NoLessSafeRedirectPolicy);
    return req;
}

void WpClient::handle(QNetworkReply* reply, ReplyHandler fn)
{
    connect(reply, &QNetworkReply::finished, this, [this, reply, fn]() {
        reply->deleteLater();
        // Transport errors (no connection, DNS failure, SSL error, etc.)
        if (reply->error() != QNetworkReply::NoError) {
            int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
            if (status == 0) {
                // Pure transport failure — no HTTP response at all
                emit networkError(reply->errorString());
                fn(0, {});
                return;
            }
        }
        int status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        fn(status, reply->readAll());
    });
}

ApiError WpClient::parse_wp_error(int status, const QByteArray& body)
{
    ApiError err;
    err.http_status = status;
    try {
        auto j      = nlohmann::json::parse(body.constBegin(), body.constEnd());
        err.code    = j.value("code",    std::string{});
        err.message = j.value("message", std::string{});
    } catch (...) {
        err.message = "HTTP " + std::to_string(status);
    }
    return err;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void WpClient::testConnection(ResultCallback<std::string> callback)
{
    auto* reply = _nam.get(make_request("/users/me"));
    handle(reply, [callback](int status, const QByteArray& body) {
        if (status != 200) { callback(false, {}, parse_wp_error(status, body)); return; }
        try {
            auto j = nlohmann::json::parse(body.constBegin(), body.constEnd());
            callback(true, j.value("name", std::string{}), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchPosts(const std::string& status, int page, PostsCallback callback)
{
    QString path = QString("/posts?per_page=50&page=%1").arg(page);
    if (!status.empty())
        path += "&status=" + QString::fromStdString(status);

    auto* reply = _nam.get(make_request(path));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200 && status_code != 206) {
            callback(false, {}, parse_wp_error(status_code, body)); return;
        }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<std::vector<WpPost>>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchPost(int64_t id, PostCallback callback)
{
    auto* reply = _nam.get(make_request(QString("/posts/%1").arg(id)));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200) { callback(false, {}, parse_wp_error(status_code, body)); return; }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<WpPost>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::createPost(const WpPost& post, PostCallback callback)
{
    nlohmann::json j = post;
    QByteArray     body = QByteArray::fromStdString(j.dump());

    auto req = make_request("/posts");
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto* reply = _nam.post(req, body);
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200 && status_code != 201) {
            callback(false, {}, parse_wp_error(status_code, body)); return;
        }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<WpPost>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::updatePost(const WpPost& post, PostCallback callback)
{
    nlohmann::json j = post;
    QByteArray     body = QByteArray::fromStdString(j.dump());

    auto req = make_request(QString("/posts/%1").arg(post.id));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    auto* reply = _nam.put(req, body);
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200) { callback(false, {}, parse_wp_error(status_code, body)); return; }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<WpPost>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::trashPost(int64_t id, VoidCallback callback)
{
    auto* reply = _nam.deleteResource(make_request(QString("/posts/%1").arg(id)));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200 && status_code != 201)
            callback(false, parse_wp_error(status_code, body));
        else
            callback(true, {});
    });
}

void WpClient::fetchMedia(int page, MediasCallback callback)
{
    auto* reply = _nam.get(make_request(QString("/media?per_page=50&page=%1").arg(page)));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200 && status_code != 206) {
            callback(false, {}, parse_wp_error(status_code, body)); return;
        }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<std::vector<WpMedia>>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::uploadMedia(const std::string& file_path, MediaCallback callback)
{
    auto* file = new QFile(QString::fromStdString(file_path));
    if (!file->open(QIODevice::ReadOnly)) {
        callback(false, {}, {0, "file_error", "Cannot open: " + file_path});
        delete file;
        return;
    }

    QFileInfo   fi(QString::fromStdString(file_path));
    QMimeDatabase db;
    QString mime     = db.mimeTypeForFile(fi).name();
    QByteArray fname = fi.fileName().toUtf8();

    auto req = make_request("/media");
    req.setHeader(QNetworkRequest::ContentTypeHeader, mime);
    req.setRawHeader("Content-Disposition", "attachment; filename=\"" + fname + "\"");

    auto* reply = _nam.post(req, file);
    file->setParent(reply); // deleted with reply

    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200 && status_code != 201) {
            callback(false, {}, parse_wp_error(status_code, body)); return;
        }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<WpMedia>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchCategories(CatsCallback callback)
{
    auto* reply = _nam.get(make_request("/categories?per_page=100"));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200) { callback(false, {}, parse_wp_error(status_code, body)); return; }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<std::vector<WpCategory>>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

void WpClient::fetchTags(TagsCallback callback)
{
    auto* reply = _nam.get(make_request("/tags?per_page=100"));
    handle(reply, [callback](int status_code, const QByteArray& body) {
        if (status_code != 200) { callback(false, {}, parse_wp_error(status_code, body)); return; }
        try {
            callback(true, nlohmann::json::parse(body.constBegin(), body.constEnd())
                              .get<std::vector<WpTag>>(), {});
        } catch (const std::exception& ex) {
            callback(false, {}, {status_code, "parse_error", ex.what()});
        }
    });
}

} // namespace wpclient
