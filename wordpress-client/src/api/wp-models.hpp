#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace wpclient {

    struct WpSite {
        std::string id;       // UUID generated locally
        std::string name;     // Display label
        std::string url;      // e.g. https://myblog.com
        std::string username; // WP username (not the password)
    };

    struct WpCategory {
        int64_t     id   = 0;
        std::string name;
        std::string slug;
    };

    struct WpTag {
        int64_t     id   = 0;
        std::string name;
        std::string slug;
    };

    struct WpPost {
        int64_t              id          = 0;
        std::string          title;
        std::string          content;     // HTML
        std::string          excerpt;
        std::string          status;      // publish | draft | future | pending
        std::string          date;        // ISO8601
        std::string          link;
        std::string          slug;
        std::vector<int64_t> category_ids;
        std::vector<int64_t> tag_ids;
        int64_t              featured_media = 0;
    };

    struct WpMedia {
        int64_t     id        = 0;
        std::string url;
        std::string filename;
        std::string mime_type;
        std::string alt_text;
        int32_t     width  = 0;
        int32_t     height = 0;
    };

    // ---- nlohmann/json deserialization ----

    inline void from_json(const nlohmann::json& j, WpCategory& c)
    {
        c.id   = j.value("id",   int64_t{0});
        c.name = j.value("name", std::string{});
        c.slug = j.value("slug", std::string{});
    }

    inline void from_json(const nlohmann::json& j, WpTag& t)
    {
        t.id   = j.value("id",   int64_t{0});
        t.name = j.value("name", std::string{});
        t.slug = j.value("slug", std::string{});
    }

    inline void from_json(const nlohmann::json& j, WpPost& p)
    {
        p.id      = j.value("id",     int64_t{0});
        p.status  = j.value("status", std::string{});
        p.date    = j.value("date",   std::string{});
        p.link    = j.value("link",   std::string{});
        p.slug    = j.value("slug",   std::string{});
        p.featured_media = j.value("featured_media", int64_t{0});

        if (j.contains("title") && j["title"].is_object())
            p.title = j["title"].value("rendered", std::string{});

        if (j.contains("content") && j["content"].is_object())
            p.content = j["content"].value("rendered", std::string{});

        if (j.contains("excerpt") && j["excerpt"].is_object())
            p.excerpt = j["excerpt"].value("rendered", std::string{});

        if (j.contains("categories") && j["categories"].is_array())
            p.category_ids = j["categories"].get<std::vector<int64_t>>();

        if (j.contains("tags") && j["tags"].is_array())
            p.tag_ids = j["tags"].get<std::vector<int64_t>>();
    }

    inline void to_json(nlohmann::json& j, const WpPost& p)
    {
        j = nlohmann::json::object();
        if (!p.title.empty())
            j["title"] = p.title;
        if (!p.content.empty())
            j["content"] = p.content;
        if (!p.excerpt.empty())
            j["excerpt"] = p.excerpt;
        if (!p.status.empty())
            j["status"] = p.status;
        if (!p.date.empty())
            j["date"] = p.date;
        if (!p.slug.empty())
            j["slug"] = p.slug;
        if (!p.category_ids.empty())
            j["categories"] = p.category_ids;
        if (!p.tag_ids.empty())
            j["tags"] = p.tag_ids;
        if (p.featured_media > 0)
            j["featured_media"] = p.featured_media;
    }

    inline void from_json(const nlohmann::json& j, WpMedia& m)
    {
        m.id        = j.value("id",        int64_t{0});
        m.mime_type = j.value("mime_type", std::string{});
        m.alt_text  = j.value("alt_text",  std::string{});

        if (j.contains("source_url"))
            m.url = j["source_url"].get<std::string>();

        if (j.contains("media_details") && j["media_details"].is_object()) {
            m.width  = j["media_details"].value("width",  0);
            m.height = j["media_details"].value("height", 0);
        }

        if (j.contains("title") && j["title"].is_object())
            m.filename = j["title"].value("rendered", std::string{});
    }

} // namespace wpclient
