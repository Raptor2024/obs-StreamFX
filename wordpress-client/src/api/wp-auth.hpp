#pragma once
#include <functional>
#include <string>

namespace wpclient {

    // Stores and retrieves Application Passwords using the system keychain
    // (QtKeychain on all platforms) or, as a fallback, encrypted QSettings.
    //
    // All operations are synchronous to keep callers simple; credential I/O
    // is fast and only happens during explicit user actions (add/remove site).
    class WpAuth {
    public:
        // Returns empty string if credentials are not stored.
        static std::string load(const std::string& site_id);

        // Persists base64(username:app_password) for the given site.
        static void store(const std::string& site_id, const std::string& username, const std::string& app_password);

        // Removes stored credentials for the site.
        static void remove(const std::string& site_id);

        // Builds the Authorization header value ready to pass to curl.
        static std::string make_auth_header(const std::string& site_id);

    private:
        static constexpr const char* SERVICE_NAME = "wp-desktop-client";

        static std::string encode_basic(const std::string& username, const std::string& app_password);
        static std::string keychain_key(const std::string& site_id);
    };

} // namespace wpclient
