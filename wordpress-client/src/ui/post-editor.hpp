#pragma once
#include "api/wp-client.hpp"
#include "api/wp-models.hpp"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QNetworkAccessManager>
#include <QPushButton>
#include <QSplitter>
#include <QTextEdit>
#include <QToolBar>
#include <QWidget>

namespace wpclient {

    class PostEditor : public QWidget {
        Q_OBJECT

    public:
        explicit PostEditor(QWidget* parent = nullptr);

        void setClient(WpClient* client);
        void openNew();
        void openPost(const WpPost& post);
        void insertMedia(const WpMedia& media);

    signals:
        void saved(const WpPost& post);
        void closed();

    private slots:
        void onBold();
        void onItalic();
        void onUnderline();
        void onLink();
        void onSaveDraft();
        void onPublish();
        void onClose();
        void onInsertMediaClicked();

    private:
        void populateTaxonomy();
        void submitPost(const std::string& status);
        void applyPost(const WpPost& post);

        WpClient*            _client  = nullptr;
        WpPost               _current;
        bool                 _is_new  = true;
        QNetworkAccessManager _nam;

        // Toolbar
        QToolBar*   _toolbar    = nullptr;

        // Title
        QLineEdit*  _title_le   = nullptr;

        // Content
        QTextEdit*  _content    = nullptr;

        // Right panel
        QComboBox*      _status_combo = nullptr;
        QDateTimeEdit*  _date_edit    = nullptr;
        QListWidget*    _cat_list     = nullptr;
        QListWidget*    _tag_list     = nullptr;
        QPushButton*    _media_btn    = nullptr;

        // Buttons
        QPushButton* _draft_btn   = nullptr;
        QPushButton* _publish_btn = nullptr;
        QPushButton* _close_btn   = nullptr;

        std::vector<WpCategory> _categories;
        std::vector<WpTag>      _tags;
    };

} // namespace wpclient
