#pragma once
#include "api/wp-client.hpp"
#include "api/wp-models.hpp"

#include <vector>

#include <QLabel>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QWidget>

namespace wpclient {

    class MediaPicker : public QWidget {
        Q_OBJECT

    public:
        explicit MediaPicker(QWidget* parent = nullptr);

        void setClient(WpClient* client);
        void load();

    signals:
        void mediaSelected(const WpMedia& media);

    private slots:
        void onUpload();
        void onCamera();
        void onInsert();
        void onLoadMore();
        void onItemDoubleClicked(QListWidgetItem* item);
        void onPhotoReady(const QString& path);

    private:
        void appendMedia(const std::vector<WpMedia>& items);
        void setBusy(bool busy);
        void uploadFile(const QString& path);

        WpClient*    _client     = nullptr;
        int          _page       = 1;
        bool         _more       = true;

        QListWidget*  _list        = nullptr;
        QPushButton*  _upload_btn  = nullptr;
        QPushButton*  _camera_btn  = nullptr;
        QPushButton*  _insert_btn  = nullptr;
        QPushButton*  _more_btn    = nullptr;
        QProgressBar* _progress    = nullptr;
        QLabel*       _status      = nullptr;

        std::vector<WpMedia> _items;
    };

} // namespace wpclient
