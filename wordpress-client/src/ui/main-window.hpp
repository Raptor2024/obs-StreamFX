#pragma once
#include "api/wp-client.hpp"
#include "api/wp-models.hpp"

#include <memory>
#include <vector>

#include <QComboBox>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QSettings>
#include <QStackedWidget>
#include <QToolBar>
#include <QWidget>

namespace wpclient {

    class PostList;
    class PostEditor;
    class MediaPicker;

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

    protected:
        void resizeEvent(QResizeEvent* event) override;

    private slots:
        void onManageSites();
        void onSiteChanged(int index);
        void onShowPosts();
        void onShowDrafts();
        void onShowMedia();
        void onNewPost();
        void onEditPost(const WpPost& post);
        void onPostSaved(const WpPost& post);
        void onEditorClosed();
        void onMediaInsert(const WpMedia& media);

    private:
        void loadSites();
        void saveSites();
        void applyCurrentSite();
        void setStatus(const QString& text);
        bool isMobileLayout() const;
        void applyLayout();

        // Sidebar (desktop) — hidden on mobile
        QWidget*       _sidebar     = nullptr;
        QComboBox*     _site_combo  = nullptr;
        QPushButton*   _posts_btn   = nullptr;
        QPushButton*   _drafts_btn  = nullptr;
        QPushButton*   _media_btn   = nullptr;
        QPushButton*   _new_btn     = nullptr;
        QPushButton*   _sites_btn   = nullptr;
        QLabel*        _status_lbl  = nullptr;

        // Bottom tab bar (mobile) — hidden on desktop
        QWidget*       _tab_bar          = nullptr;
        QPushButton*   _tab_posts_btn    = nullptr;
        QPushButton*   _tab_drafts_btn   = nullptr;
        QPushButton*   _tab_media_btn    = nullptr;
        QPushButton*   _tab_new_btn      = nullptr;
        QPushButton*   _tab_sites_btn    = nullptr;
        QComboBox*     _tab_site_combo   = nullptr;

        // Central content area
        QStackedWidget* _stack      = nullptr;
        PostList*        _post_list = nullptr;
        PostEditor*      _editor    = nullptr;
        MediaPicker*     _media_page = nullptr;

        std::vector<WpSite>         _sites;
        std::unique_ptr<WpClient>   _client;
        QSettings                   _settings;
    };

} // namespace wpclient
