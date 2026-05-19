#pragma once
#include <QDialog>
#include <QCamera>
#include <QImageCapture>
#include <QMediaCaptureSession>
#include <QVideoWidget>
#include <QPushButton>
#include <QLabel>

namespace wpclient {

    // Full-screen camera viewfinder + capture button.
    // On mobile this occupies the whole screen; on desktop it's a dialog.
    // Emits photoCaptured(path) with the saved temp-file path on success.
    class CameraCapture : public QDialog {
        Q_OBJECT

    public:
        explicit CameraCapture(QWidget* parent = nullptr);
        ~CameraCapture() override;

    signals:
        void photoCaptured(const QString& file_path);

    private slots:
        void onCapture();
        void onImageSaved(int id, const QString& path);
        void onCameraError(QCamera::Error error, const QString& msg);

    private:
        QCamera*               _camera   = nullptr;
        QImageCapture*         _capture  = nullptr;
        QMediaCaptureSession   _session;
        QVideoWidget*          _viewfinder = nullptr;
        QPushButton*           _shutter    = nullptr;
        QLabel*                _status     = nullptr;
    };

} // namespace wpclient
