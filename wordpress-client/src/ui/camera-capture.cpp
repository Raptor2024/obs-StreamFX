#include "camera-capture.hpp"

#include <QDir>
#include <QMediaDevices>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QHBoxLayout>

namespace wpclient {

CameraCapture::CameraCapture(QWidget* parent)
    : QDialog(parent, Qt::Window)
{
    setWindowTitle("Take Photo");

    // Full-screen on mobile
#if defined(Q_OS_ANDROID) || defined(Q_OS_IOS)
    showFullScreen();
#else
    resize(640, 480);
#endif

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _viewfinder = new QVideoWidget;
    layout->addWidget(_viewfinder, 1);

    _status = new QLabel;
    _status->setAlignment(Qt::AlignCenter);
    _status->setStyleSheet("color: white; background: rgba(0,0,0,0.5); padding: 4px;");
    _status->setVisible(false);

    auto* bar = new QWidget;
    bar->setStyleSheet("background: black;");
    auto* bar_layout = new QHBoxLayout(bar);
    bar_layout->setContentsMargins(12, 12, 12, 12);

    auto* cancel = new QPushButton("Cancel");
    cancel->setStyleSheet("color: white; background: transparent; border: 1px solid white; border-radius: 6px; padding: 8px 16px;");

    _shutter = new QPushButton("Capture");
    _shutter->setStyleSheet("color: black; background: white; border-radius: 6px; padding: 8px 24px; font-weight: bold;");

    bar_layout->addWidget(cancel);
    bar_layout->addStretch();
    bar_layout->addWidget(_status);
    bar_layout->addStretch();
    bar_layout->addWidget(_shutter);
    layout->addWidget(bar);

    // Set up camera
    _camera  = new QCamera(QMediaDevices::defaultVideoInput(), this);
    _capture = new QImageCapture(this);
    _session.setCamera(_camera);
    _session.setImageCapture(_capture);
    _session.setVideoOutput(_viewfinder);

    connect(_shutter, &QPushButton::clicked, this, &CameraCapture::onCapture);
    connect(cancel,   &QPushButton::clicked, this, &QDialog::reject);
    connect(_capture, &QImageCapture::imageSaved,  this, &CameraCapture::onImageSaved);
    connect(_camera,  &QCamera::errorOccurred,     this, &CameraCapture::onCameraError);

    _camera->start();
}

CameraCapture::~CameraCapture()
{
    _camera->stop();
}

void CameraCapture::onCapture()
{
    _shutter->setEnabled(false);
    _status->setText("Capturing...");
    _status->setVisible(true);

    QString dir = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    _capture->captureToFile(dir + "/wp_capture.jpg");
}

void CameraCapture::onImageSaved(int, const QString& path)
{
    emit photoCaptured(path);
    accept();
}

void CameraCapture::onCameraError(QCamera::Error, const QString& msg)
{
    _status->setText("Camera error: " + msg);
    _status->setVisible(true);
    _shutter->setEnabled(true);
}

} // namespace wpclient
