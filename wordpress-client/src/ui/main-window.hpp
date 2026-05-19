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

namespace wpclient {

    class PostList;
    class PostEditor;
    class MediaPicker;

    class MainWindow : public QMainWindow {
        Q_OBJECT

    public:
        explicit MainWindow(QWidget* parent = nullptr);
        ~MainWindow() override;

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

        // Sidebar / navigation
        QComboBox*     _site_combo  = nullptr;
        QPushButton*   _posts_btn   = nullptr;
        QPushButton*   _drafts_btn  = nullptr;
        QPushButton*   _media_btn   = nullptr;
        QPushButton*   _new_btn     = nullptr;
        QPushButton*   _sites_btn   = nullptr;
        QLabel*        _status_lbl  = nullptr;

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
