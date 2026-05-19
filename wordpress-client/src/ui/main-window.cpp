#include "main-window.hpp"
#include "site-manager.hpp"
#include "post-list.hpp"
#include "post-editor.hpp"
#include "media-picker.hpp"

#include <QApplication>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QUuid>

namespace wpclient {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , _settings("wp-desktop-client", "app")
{
    setWindowTitle("WordPress Desktop Client");
    setMinimumSize(900, 600);

    // ---- Sidebar ----
    auto* sidebar = new QWidget;
    auto* sbl     = new QVBoxLayout(sidebar);
    sbl->setContentsMargins(8, 8, 8, 8);
    sbl->setSpacing(6);
    sidebar->setFixedWidth(180);

    _site_combo = new QComboBox;
    _posts_btn  = new QPushButton("Posts");
    _drafts_btn = new QPushButton("Drafts");
    _media_btn  = new QPushButton("Media");
    _new_btn    = new QPushButton("+ New Post");
    _sites_btn  = new QPushButton("Manage Sites...");
    _status_lbl = new QLabel;
    _status_lbl->setWordWrap(true);
    _status_lbl->setStyleSheet("color: gray; font-size: 11px;");

    for (auto* btn : {_posts_btn, _drafts_btn, _media_btn}) {
        btn->setCheckable(true);
        btn->setFlat(true);
        btn->setStyleSheet("QPushButton { text-align: left; padding: 6px; border-radius: 4px; }"
                           "QPushButton:checked { background: palette(highlight); color: palette(highlighted-text); }");
    }
    _new_btn->setDefault(true);

    sbl->addWidget(_site_combo);
    sbl->addSpacing(8);
    sbl->addWidget(_new_btn);
    sbl->addSpacing(4);
    sbl->addWidget(_posts_btn);
    sbl->addWidget(_drafts_btn);
    sbl->addWidget(_media_btn);
    sbl->addStretch();
    sbl->addWidget(_status_lbl);
    sbl->addWidget(_sites_btn);

    // ---- Central stack ----
    _post_list  = new PostList;
    _editor     = new PostEditor;
    _media_page = new MediaPicker;

    _stack = new QStackedWidget;
    _stack->addWidget(_post_list);   // index 0
    _stack->addWidget(_editor);      // index 1
    _stack->addWidget(_media_page);  // index 2

    // ---- Splitter ----
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(sidebar);
    splitter->addWidget(_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setChildrenCollapsible(false);
    setCentralWidget(splitter);

    // ---- Status bar ----
    statusBar()->showMessage("Ready");

    // ---- Connections ----
    connect(_site_combo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSiteChanged);
    connect(_posts_btn,   &QPushButton::clicked, this, &MainWindow::onShowPosts);
    connect(_drafts_btn,  &QPushButton::clicked, this, &MainWindow::onShowDrafts);
    connect(_media_btn,   &QPushButton::clicked, this, &MainWindow::onShowMedia);
    connect(_new_btn,     &QPushButton::clicked, this, &MainWindow::onNewPost);
    connect(_sites_btn,   &QPushButton::clicked, this, &MainWindow::onManageSites);
    connect(_post_list,   &PostList::editRequested, this, &MainWindow::onEditPost);
    connect(_editor,      &PostEditor::saved,   this, &MainWindow::onPostSaved);
    connect(_editor,      &PostEditor::closed,  this, &MainWindow::onEditorClosed);
    connect(_media_page,  &MediaPicker::mediaSelected, this, &MainWindow::onMediaInsert);

    loadSites();
    _posts_btn->setChecked(true);
}

MainWindow::~MainWindow()
{
    saveSites();
}

void MainWindow::loadSites()
{
    _settings.beginGroup("sites");
    QStringList ids = _settings.childGroups();
    _sites.clear();
    for (const auto& id : ids) {
        _settings.beginGroup(id);
        WpSite s;
        s.id       = id.toStdString();
        s.name     = _settings.value("name").toString().toStdString();
        s.url      = _settings.value("url").toString().toStdString();
        s.username = _settings.value("username").toString().toStdString();
        _sites.push_back(std::move(s));
        _settings.endGroup();
    }
    _settings.endGroup();

    _site_combo->blockSignals(true);
    _site_combo->clear();
    for (const auto& s : _sites)
        _site_combo->addItem(QString::fromStdString(s.name));
    _site_combo->blockSignals(false);

    if (!_sites.empty())
        applyCurrentSite();
    else
        setStatus("No sites configured.\nUse \"Manage Sites\" to add one.");
}

void MainWindow::saveSites()
{
    _settings.beginGroup("sites");
    _settings.remove(""); // clear all children
    for (const auto& s : _sites) {
        _settings.beginGroup(QString::fromStdString(s.id));
        _settings.setValue("name",     QString::fromStdString(s.name));
        _settings.setValue("url",      QString::fromStdString(s.url));
        _settings.setValue("username", QString::fromStdString(s.username));
        _settings.endGroup();
    }
    _settings.endGroup();
}

void MainWindow::applyCurrentSite()
{
    int idx = _site_combo->currentIndex();
    if (idx < 0 || idx >= static_cast<int>(_sites.size()))
        return;

    const WpSite& site = _sites[static_cast<size_t>(idx)];
    _client = std::make_unique<WpClient>(site);
    connect(_client.get(), &WpClient::networkError, this, [this](const QString& msg) {
        statusBar()->showMessage("Network error: " + msg, 5000);
    });

    _post_list->setClient(_client.get());
    _editor->setClient(_client.get());
    _media_page->setClient(_client.get());

    onShowPosts();
    setStatus(QString::fromStdString(site.name));
}

void MainWindow::setStatus(const QString& text)
{
    _status_lbl->setText(text);
}

void MainWindow::onManageSites()
{
    SiteManager dlg(_sites, this);
    if (dlg.exec() == QDialog::Accepted) {
        _sites = dlg.sites();
        saveSites();
        loadSites();
    }
}

void MainWindow::onSiteChanged(int)
{
    applyCurrentSite();
}

void MainWindow::onShowPosts()
{
    _posts_btn->setChecked(true);
    _drafts_btn->setChecked(false);
    _media_btn->setChecked(false);
    _stack->setCurrentWidget(_post_list);
    _post_list->loadPosts("publish");
}

void MainWindow::onShowDrafts()
{
    _posts_btn->setChecked(false);
    _drafts_btn->setChecked(true);
    _media_btn->setChecked(false);
    _stack->setCurrentWidget(_post_list);
    _post_list->loadPosts("draft");
}

void MainWindow::onShowMedia()
{
    _posts_btn->setChecked(false);
    _drafts_btn->setChecked(false);
    _media_btn->setChecked(true);
    _stack->setCurrentWidget(_media_page);
    _media_page->load();
}

void MainWindow::onNewPost()
{
    _stack->setCurrentWidget(_editor);
    _editor->openNew();
}

void MainWindow::onEditPost(const WpPost& post)
{
    _stack->setCurrentWidget(_editor);
    _editor->openPost(post);
}

void MainWindow::onPostSaved(const WpPost& post)
{
    statusBar()->showMessage(QString("Saved: %1").arg(QString::fromStdString(post.title)), 3000);
    onEditorClosed();
}

void MainWindow::onEditorClosed()
{
    // Return to whichever list was active
    if (_drafts_btn->isChecked())
        onShowDrafts();
    else
        onShowPosts();
}

void MainWindow::onMediaInsert(const WpMedia& media)
{
    // If editor is open, forward the selection; otherwise just switch to editor
    if (_stack->currentWidget() == _editor)
        _editor->insertMedia(media);
    else
        _stack->setCurrentWidget(_editor);
}

} // namespace wpclient
