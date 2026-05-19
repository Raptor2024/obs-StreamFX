#include "wp-auth.hpp"

#include <QByteArray>
#include <QSettings>
#include <QString>

#ifdef HAS_QTKEYCHAIN
#include <qt6keychain/keychain.h>
#endif

namespace wpclient {

std::string WpAuth::keychain_key(const std::string& site_id)
{
    return "site/" + site_id + "/token";
}

std::string WpAuth::encode_basic(const std::string& username, const std::string& app_password)
{
    // RFC 7617: credentials = username ":" password, base64-encoded
    std::string raw = username + ":" + app_password;
    QByteArray  encoded = QByteArray(raw.c_str(), static_cast<qsizetype>(raw.size())).toBase64();
    // Overwrite the raw credentials in memory before returning
    std::fill(raw.begin(), raw.end(), '\0');
    return std::string("Basic ") + encoded.constData();
}

std::string WpAuth::load(const std::string& site_id)
{
#ifdef HAS_QTKEYCHAIN
    QKeychain::ReadPasswordJob job(QString::fromStdString(SERVICE_NAME));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keychain_key(site_id)));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();

    if (job.error() == QKeychain::NoError)
        return job.textData().toStdString();
    return {};
#else
    QSettings settings(QString::fromStdString(SERVICE_NAME), "credentials");
    QString key = QString::fromStdString(keychain_key(site_id));
    if (!settings.contains(key))
        return {};
    // Fallback: stored as base64 of already-base64-encoded token (obfuscation only)
    QByteArray raw = QByteArray::fromBase64(settings.value(key).toByteArray());
    return raw.toStdString();
#endif
}

void WpAuth::store(const std::string& site_id, const std::string& username, const std::string& app_password)
{
    std::string token = encode_basic(username, app_password);

#ifdef HAS_QTKEYCHAIN
    QKeychain::WritePasswordJob job(QString::fromStdString(SERVICE_NAME));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keychain_key(site_id)));
    job.setTextData(QString::fromStdString(token));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
#else
    QSettings settings(QString::fromStdString(SERVICE_NAME), "credentials");
    QByteArray encoded = QByteArray(token.c_str(), static_cast<qsizetype>(token.size())).toBase64();
    settings.setValue(QString::fromStdString(keychain_key(site_id)), encoded);
#endif

    // Zero out the in-memory token copy
    std::fill(token.begin(), token.end(), '\0');
}

void WpAuth::remove(const std::string& site_id)
{
#ifdef HAS_QTKEYCHAIN
    QKeychain::DeletePasswordJob job(QString::fromStdString(SERVICE_NAME));
    job.setAutoDelete(false);
    job.setKey(QString::fromStdString(keychain_key(site_id)));

    QEventLoop loop;
    QObject::connect(&job, &QKeychain::Job::finished, &loop, &QEventLoop::quit);
    job.start();
    loop.exec();
#else
    QSettings settings(QString::fromStdString(SERVICE_NAME), "credentials");
    settings.remove(QString::fromStdString(keychain_key(site_id)));
#endif
}

std::string WpAuth::make_auth_header(const std::string& site_id)
{
    return load(site_id);
}

} // namespace wpclient
