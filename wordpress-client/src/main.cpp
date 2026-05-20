#include "ui/main-window.hpp"
#include <QApplication>

static const char* QSS = R"(
QMainWindow { background: #f0f0f1; }
QDialog     { background: #f6f7f7; }

/* ── Sidebar ─────────────────────────────────────────────── */
QWidget#sidebar { background: #23282d; }

QLabel#sidebar-logo {
    color: white;
    font-size: 15px;
    font-weight: bold;
    padding: 6px 4px 14px 4px;
    border-bottom: 1px solid #40464d;
}

QComboBox#site-combo {
    background: #32373c;
    color: #ccd2d7;
    border: 1px solid #40464d;
    border-radius: 3px;
    padding: 5px 8px;
}
QComboBox#site-combo::drop-down { border: none; width: 18px; }
QComboBox#site-combo QAbstractItemView {
    background: #32373c;
    color: #ccd2d7;
    selection-background-color: #0073aa;
    border: 1px solid #40464d;
}

QPushButton#nav-btn {
    color: #a7aaad;
    background: transparent;
    border: none;
    border-radius: 3px;
    padding: 8px 10px;
    text-align: left;
}
QPushButton#nav-btn:hover   { background: #32373c; color: #72aee6; }
QPushButton#nav-btn:checked { background: #0073aa; color: white; }

QPushButton#new-post-btn {
    background: #0073aa;
    color: white;
    border: none;
    border-radius: 3px;
    padding: 9px 12px;
    font-weight: bold;
}
QPushButton#new-post-btn:hover   { background: #006799; }
QPushButton#new-post-btn:pressed { background: #005177; }

QPushButton#manage-sites-btn {
    color: #72aee6;
    background: transparent;
    border: none;
    padding: 6px 4px;
    text-align: left;
    font-size: 12px;
}
QPushButton#manage-sites-btn:hover { color: white; }

QLabel#site-status {
    color: #72aee6;
    font-size: 11px;
    padding: 2px 4px;
}

/* ── Mobile tab bar ──────────────────────────────────────── */
QWidget#tab-bar { background: #23282d; }

QPushButton#tab-btn {
    color: #a7aaad;
    background: transparent;
    border: none;
    border-radius: 0;
    font-size: 10px;
    padding: 6px 4px;
}
QPushButton#tab-btn:checked { color: #00a0d2; }
QPushButton#tab-btn:hover   { color: white; }

/* ── Post list table ─────────────────────────────────────── */
QTableWidget {
    background: white;
    gridline-color: #f0f0f1;
    border: none;
    selection-background-color: #f0f5fa;
    selection-color: #1d2327;
    alternate-background-color: #f9f9f9;
}
QHeaderView::section {
    background: #f6f7f7;
    color: #50575e;
    border: none;
    border-bottom: 2px solid #dcdcde;
    padding: 6px 8px;
    font-weight: bold;
    font-size: 12px;
}

QPushButton#load-more-btn {
    background: transparent;
    color: #0073aa;
    border: 1px solid #0073aa;
    border-radius: 3px;
    padding: 6px 20px;
}
QPushButton#load-more-btn:hover { background: #f0f5fa; }

/* ── Editor toolbar ──────────────────────────────────────── */
QToolBar {
    background: #f6f7f7;
    border: none;
    border-bottom: 1px solid #dcdcde;
    padding: 3px;
    spacing: 2px;
}
QToolBar QToolButton {
    background: transparent;
    border: 1px solid transparent;
    border-radius: 3px;
    padding: 4px 9px;
}
QToolBar QToolButton:hover   { background: #dcdcde; border-color: #c3c4c7; }
QToolBar QToolButton:pressed { background: #c3c4c7; }

/* ── Post title ──────────────────────────────────────────── */
QLineEdit#post-title {
    font-size: 20px;
    font-weight: bold;
    padding: 10px 12px;
    border: none;
    border-bottom: 2px solid #dcdcde;
    background: white;
    color: #1d2327;
}
QLineEdit#post-title:focus { border-bottom-color: #0073aa; }

/* ── Content area ────────────────────────────────────────── */
QTextEdit {
    background: white;
    border: none;
    padding: 8px 12px;
    font-size: 14px;
    color: #1d2327;
}

/* ── Editor right-panel group boxes ─────────────────────── */
QGroupBox {
    font-weight: bold;
    font-size: 12px;
    color: #50575e;
    border: 1px solid #dcdcde;
    border-radius: 4px;
    margin-top: 10px;
    padding-top: 6px;
}
QGroupBox::title {
    subcontrol-origin: margin;
    subcontrol-position: top left;
    left: 10px;
    padding: 0 4px;
    background: white;
}

/* ── Action buttons ──────────────────────────────────────── */
QPushButton#publish-btn {
    background: #0073aa;
    color: white;
    border: none;
    border-radius: 3px;
    padding: 8px 22px;
    font-weight: bold;
    font-size: 13px;
    min-height: 32px;
}
QPushButton#publish-btn:hover   { background: #006799; }
QPushButton#publish-btn:pressed { background: #005177; }

QPushButton#draft-btn {
    background: white;
    color: #0073aa;
    border: 1px solid #0073aa;
    border-radius: 3px;
    padding: 7px 18px;
    font-size: 13px;
    min-height: 32px;
}
QPushButton#draft-btn:hover { background: #f0f5fa; }

QPushButton#close-btn {
    background: transparent;
    color: #72aee6;
    border: none;
    padding: 8px 10px;
    font-size: 13px;
}
QPushButton#close-btn:hover { color: #0073aa; }

QPushButton#insert-media-btn {
    background: white;
    color: #0073aa;
    border: 1px solid #0073aa;
    border-radius: 3px;
    padding: 4px 12px;
    font-size: 12px;
}
QPushButton#insert-media-btn:hover { background: #f0f5fa; }

/* ── Status bar ──────────────────────────────────────────── */
QStatusBar {
    background: #f0f0f1;
    border-top: 1px solid #dcdcde;
    font-size: 11px;
    color: #50575e;
}

/* ── Scrollbars ──────────────────────────────────────────── */
QScrollBar:vertical {
    background: #f0f0f1;
    width: 8px;
    border-radius: 4px;
    margin: 0;
}
QScrollBar::handle:vertical {
    background: #c3c4c7;
    border-radius: 4px;
    min-height: 24px;
}
QScrollBar::handle:vertical:hover             { background: #a7aaad; }
QScrollBar::add-line:vertical,
QScrollBar::sub-line:vertical                 { height: 0; }
QScrollBar:horizontal {
    background: #f0f0f1;
    height: 8px;
    border-radius: 4px;
    margin: 0;
}
QScrollBar::handle:horizontal {
    background: #c3c4c7;
    border-radius: 4px;
    min-width: 24px;
}
QScrollBar::handle:horizontal:hover           { background: #a7aaad; }
QScrollBar::add-line:horizontal,
QScrollBar::sub-line:horizontal               { width: 0; }

/* ── Splitter ────────────────────────────────────────────── */
QSplitter::handle:horizontal { background: #dcdcde; width: 1px; }
QSplitter::handle:vertical   { background: #dcdcde; height: 1px; }
)";

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("WordPress Desktop");
    app.setOrganizationName("wp-desktop-client");
    app.setApplicationVersion("1.0.0");
    app.setStyleSheet(QSS);

    wpclient::MainWindow window;
    window.show();

    return app.exec();
}
