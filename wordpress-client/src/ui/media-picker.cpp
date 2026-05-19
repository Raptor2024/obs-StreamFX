#include "media-picker.hpp"
#include "camera-capture.hpp"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QMediaDevices>
#include <QMessageBox>
#include <QVBoxLayout>

namespace wpclient {

MediaPicker::MediaPicker(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    _list = new QListWidget;
    _list->setViewMode(QListWidget::IconMode);
    _list->setIconSize(QSize(120, 90));
    _list->setResizeMode(QListWidget::Adjust);
    _list->setMovement(QListWidget::Static);
    _list->setSpacing(6);
    layout->addWidget(_list, 1);

    _progress = new QProgressBar;
    _progress->setRange(0, 0); // indeterminate
    _progress->setVisible(false);
    layout->addWidget(_progress);

    _status = new QLabel;
    _status->setStyleSheet("color: gray;");
    layout->addWidget(_status);

    auto* btn_row  = new QHBoxLayout;
    _upload_btn    = new QPushButton("Upload File...");
    _camera_btn    = new QPushButton("Take Photo");
    _insert_btn    = new QPushButton("Insert Selected");
    _more_btn      = new QPushButton("Load more...");
    _insert_btn->setEnabled(false);
    _more_btn->setVisible(false);

    // Hide camera button on platforms with no cameras
    _camera_btn->setVisible(!QMediaDevices::videoInputs().isEmpty());

    btn_row->addWidget(_upload_btn);
    btn_row->addWidget(_camera_btn);
    btn_row->addStretch();
    btn_row->addWidget(_more_btn);
    btn_row->addWidget(_insert_btn);
    layout->addLayout(btn_row);

    connect(_upload_btn, &QPushButton::clicked,           this, &MediaPicker::onUpload);
    connect(_camera_btn, &QPushButton::clicked,           this, &MediaPicker::onCamera);
    connect(_insert_btn, &QPushButton::clicked,           this, &MediaPicker::onInsert);
    connect(_more_btn,   &QPushButton::clicked,           this, &MediaPicker::onLoadMore);
    connect(_list,       &QListWidget::itemDoubleClicked, this, &MediaPicker::onItemDoubleClicked);
    connect(_list,       &QListWidget::itemSelectionChanged, this, [this]() {
        _insert_btn->setEnabled(!_list->selectedItems().isEmpty());
    });
}

void MediaPicker::setClient(WpClient* client)
{
    _client = client;
}

void MediaPicker::load()
{
    if (!_client)
        return;
    _page  = 1;
    _more  = true;
    _items.clear();
    _list->clear();
    _status->clear();
    _more_btn->setVisible(false);

    setBusy(true);
    _client->fetchMedia(_page, [this](bool ok, std::vector<WpMedia> items, ApiError err) {
        setBusy(false);
        if (!ok) {
            _status->setText("Error: " + QString::fromStdString(err.message));
            return;
        }
        appendMedia(items);
        _more = (static_cast<int>(items.size()) == 50);
        _more_btn->setVisible(_more);
        if (_items.empty())
            _status->setText("No media files found.");
    });
}

void MediaPicker::onLoadMore()
{
    if (!_client || !_more)
        return;
    ++_page;
    setBusy(true);
    _client->fetchMedia(_page, [this](bool ok, std::vector<WpMedia> items, ApiError err) {
        setBusy(false);
        if (!ok) {
            _status->setText("Error: " + QString::fromStdString(err.message));
            return;
        }
        appendMedia(items);
        _more = (static_cast<int>(items.size()) == 50);
        _more_btn->setVisible(_more);
    });
}

void MediaPicker::appendMedia(const std::vector<WpMedia>& items)
{
    for (const auto& m : items) {
        auto* item = new QListWidgetItem(QString::fromStdString(m.filename));
        item->setData(Qt::UserRole, static_cast<qlonglong>(_items.size()));
        item->setToolTip(QString("%1\n%2 × %3\n%4")
            .arg(QString::fromStdString(m.url))
            .arg(m.width).arg(m.height)
            .arg(QString::fromStdString(m.mime_type)));
        _list->addItem(item);
        _items.push_back(m);
    }
}

void MediaPicker::onUpload()
{
    if (!_client)
        return;

    QString path = QFileDialog::getOpenFileName(
        this, "Select file to upload", {},
        "Images (*.png *.jpg *.jpeg *.gif *.webp);;Videos (*.mp4 *.mov *.webm);;All files (*)");

    if (path.isEmpty())
        return;

    uploadFile(path);
}

void MediaPicker::onCamera()
{
    auto* cam = new CameraCapture(this);
    connect(cam, &CameraCapture::photoCaptured, this, &MediaPicker::onPhotoReady);
    cam->exec();
}

void MediaPicker::onPhotoReady(const QString& path)
{
    uploadFile(path);
}

void MediaPicker::uploadFile(const QString& path)
{
    if (!_client || path.isEmpty())
        return;

    setBusy(true);
    _status->setText("Uploading...");

    _client->uploadMedia(path.toStdString(), [this](bool ok, WpMedia media, ApiError err) {
        setBusy(false);
        if (!ok) {
            _status->setText("Upload failed: " + QString::fromStdString(err.message));
            QMessageBox::critical(this, "Upload Error", QString::fromStdString(err.message));
            return;
        }
        _status->setText("Uploaded: " + QString::fromStdString(media.filename));
        appendMedia({media});
    });
}

void MediaPicker::onInsert()
{
    auto* item = _list->currentItem();
    if (!item)
        return;
    size_t idx = static_cast<size_t>(item->data(Qt::UserRole).toLongLong());
    if (idx < _items.size())
        emit mediaSelected(_items[idx]);
}

void MediaPicker::onItemDoubleClicked(QListWidgetItem* item)
{
    size_t idx = static_cast<size_t>(item->data(Qt::UserRole).toLongLong());
    if (idx < _items.size())
        emit mediaSelected(_items[idx]);
}

void MediaPicker::setBusy(bool busy)
{
    _progress->setVisible(busy);
    _upload_btn->setEnabled(!busy);
    _camera_btn->setEnabled(!busy);
    _more_btn->setEnabled(!busy);
    _list->setEnabled(!busy);
}

} // namespace wpclient
