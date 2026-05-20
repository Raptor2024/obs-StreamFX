#include "main-window.hpp"
#include "site-manager.hpp"
#include "post-list.hpp"
#include "post-editor.hpp"
#include "media-picker.hpp"

#include <QApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QResizeEvent>
#include <QScreen>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace wpclient {

// Screens narrower than this (logical pixels) switch to mobile layout
static constexpr int MOBILE_BREAKPOINT = 600;

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , _settings("wp-desktop-client", "app")
{
    setWindowTitle("WordPress Desktop");
    setMinimumSize(320, 480);

    // ---- Desktop sidebar ----
    _sidebar = new QWidget;
    _sidebar->setObjectName("sidebar");
    _sidebar->setAutoFillBackground(true);
    auto* sbl = new QVBoxLayout(_sidebar);
    sbl->setContentsMargins(10, 10, 10, 10);
    sbl->setSpacing(4);
    _sidebar->setFixedWidth(190);

    auto* logo_lbl = new QLabel("WordPress");
    logo_lbl->setObjectName("sidebar-logo");
    logo_lbl->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    _site_combo = new QComboBox;
    _site_combo->setObjectName("site-combo");
    _posts_btn  = new QPushButton("Posts");
    _drafts_btn = new QPushButton("Drafts");
    _media_btn  = new QPushButton("Media");
    _new_btn    = new QPushButton("+ New Post");
    _sites_btn  = new QPushButton("Manage Sites...");
    _status_lbl = new QLabel;
    _status_lbl->setObjectName("site-status");
    _status_lbl->setWordWrap(true);

    for (auto* btn : {_posts_btn, _drafts_btn, _media_btn}) {
        btn->setObjectName("nav-btn");
        btn->setCheckable(true);
        btn->setFlat(true);
    }
    _new_btn->setObjectName("new-post-btn");
    _sites_btn->setObjectName("manage-sites-btn");

    sbl->addWidget(logo_lbl);
    sbl->addSpacing(6);
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

    // ---- Mobile bottom tab bar ----
    _tab_bar = new QWidget;
    _tab_bar->setObjectName("tab-bar");
    _tab_bar->setAutoFillBackground(true);
    auto* tab_layout = new QHBoxLayout(_tab_bar);
    tab_layout->setContentsMargins(0, 0, 0, 0);
    tab_layout->setSpacing(0);

    auto make_tab = [](const QString& icon, const QString& label) {
        auto* btn = new QPushButton(icon + "\n" + label);
        btn->setObjectName("tab-btn");
        btn->setCheckable(true);
        btn->setFlat(true);
        return btn;
    };

    _tab_posts_btn  = make_tab("📝", "Posts");
    _tab_drafts_btn = make_tab("📄", "Drafts");
    _tab_new_btn    = make_tab("＋", "New");
    _tab_media_btn  = make_tab("🖼", "Media");
    _tab_sites_btn  = make_tab("⚙", "Sites");

    tab_layout->addWidget(_tab_posts_btn);
    tab_layout->addWidget(_tab_drafts_btn);
    tab_layout->addWidget(_tab_new_btn);
    tab_layout->addWidget(_tab_media_btn);
    tab_layout->addWidget(_tab_sites_btn);

    // Mobile site selector (appears above tab bar)
    _tab_site_combo = new QComboBox;

    // ---- Central stack ----
    _post_list  = new PostList;
    _editor     = new PostEditor;
    _media_page = new MediaPicker;

    _stack = new QStackedWidget;
    _stack->addWidget(_post_list);   // 0
    _stack->addWidget(_editor);      // 1
    _stack->addWidget(_media_page);  // 2

    // ---- Splitter (desktop) ----
    auto* splitter = new QSplitter(Qt::Horizontal);
    splitter->addWidget(_sidebar);
    splitter->addWidget(_stack);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setChildrenCollapsible(false);

    // ---- Root layout ----
    auto* root     = new QWidget;
    auto* root_vl  = new QVBoxLayout(root);
    root_vl->setContentsMargins(0, 0, 0, 0);
    root_vl->setSpacing(0);
    root_vl->addWidget(_tab_site_combo); // hidden on desktop
    root_vl->addWidget(splitter, 1);
    root_vl->addWidget(_tab_bar);        // hidden on desktop
    setCentralWidget(root);

    statusBar()->showMessage("Ready");

    // ---- Connections: desktop sidebar ----
    connect(_site_combo,  QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSiteChanged);
    connect(_posts_btn,   &QPushButton::clicked, this, &MainWindow::onShowPosts);
    connect(_drafts_btn,  &QPushButton::clicked, this, &MainWindow::onShowDrafts);
    connect(_media_btn,   &QPushButton::clicked, this, &MainWindow::onShowMedia);
    connect(_new_btn,     &QPushButton::clicked, this, &MainWindow::onNewPost);
    connect(_sites_btn,   &QPushButton::clicked, this, &MainWindow::onManageSites);

    // ---- Connections: mobile tab bar ----
    connect(_tab_site_combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onSiteChanged);
    connect(_tab_posts_btn,  &QPushButton::clicked, this, &MainWindow::onShowPosts);
    connect(_tab_drafts_btn, &QPushButton::clicked, this, &MainWindow::onShowDrafts);
    connect(_tab_new_btn,    &QPushButton::clicked, this, &MainWindow::onNewPost);
    connect(_tab_media_btn,  &QPushButton::clicked, this, &MainWindow::onShowMedia);
    connect(_tab_sites_btn,  &QPushButton::clicked, this, &MainWindow::onManageSites);

    // ---- Connections: content widgets ----
    connect(_post_list,   &PostList::editRequested,    this, &MainWindow::onEditPost);
    connect(_editor,      &PostEditor::saved,          this, &MainWindow::onPostSaved);
    connect(_editor,      &PostEditor::closed,         this, &MainWindow::onEditorClosed);
    connect(_media_page,  &MediaPicker::mediaSelected, this, &MainWindow::onMediaInsert);

    loadSites();
    _posts_btn->setChecked(true);
    _tab_posts_btn->setChecked(true);
    applyLayout();
}

MainWindow::~MainWindow()
{
    saveSites();
}

bool MainWindow::isMobileLayout() const
{
    return width() < MOBILE_BREAKPOINT;
}

void MainWindow::applyLayout()
{
    bool mobile = isMobileLayout();
    _sidebar->setVisible(!mobile);
    _tab_bar->setVisible(mobile);
    _tab_site_combo->setVisible(mobile);

    // Keep both combos in sync
    auto sync_combo = [&](QComboBox* src, QComboBox* dst) {
        dst->blockSignals(true);
        dst->clear();
        for (int i = 0; i < src->count(); ++i)
            dst->addItem(src->itemText(i));
        dst->setCurrentIndex(src->currentIndex());
        dst->blockSignals(false);
    };
    sync_combo(_site_combo, _tab_site_combo);
}

void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    applyLayout();
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

    for (auto* combo : {_site_combo, _tab_site_combo}) {
        combo->blockSignals(true);
        combo->clear();
        for (const auto& s : _sites)
            combo->addItem(QString::fromStdString(s.name));
        combo->blockSignals(false);
    }

    if (!_sites.empty())
        applyCurrentSite();
    else
        setStatus("No sites configured.");
}

void MainWindow::saveSites()
{
    _settings.beginGroup("sites");
    _settings.remove("");
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
    // Keep both combos in sync with whichever fired
    auto* src = qobject_cast<QComboBox*>(sender());
    auto* dst = (src == _site_combo) ? _tab_site_combo : _site_combo;
    if (dst) {
        dst->blockSignals(true);
        dst->setCurrentIndex(src->currentIndex());
        dst->blockSignals(false);
    }
    applyCurrentSite();
}

static void setTabChecked(QPushButton* p, QPushButton* d, QPushButton* m, QPushButton* active)
{
    p->setChecked(active == p);
    d->setChecked(active == d);
    m->setChecked(active == m);
}

void MainWindow::onShowPosts()
{
    _posts_btn->setChecked(true);  _drafts_btn->setChecked(false); _media_btn->setChecked(false);
    setTabChecked(_tab_posts_btn, _tab_drafts_btn, _tab_media_btn, _tab_posts_btn);
    _stack->setCurrentWidget(_post_list);
    _post_list->loadPosts("publish");
}

void MainWindow::onShowDrafts()
{
    _posts_btn->setChecked(false); _drafts_btn->setChecked(true);  _media_btn->setChecked(false);
    setTabChecked(_tab_posts_btn, _tab_drafts_btn, _tab_media_btn, _tab_drafts_btn);
    _stack->setCurrentWidget(_post_list);
    _post_list->loadPosts("draft");
}

void MainWindow::onShowMedia()
{
    _posts_btn->setChecked(false); _drafts_btn->setChecked(false); _media_btn->setChecked(true);
    setTabChecked(_tab_posts_btn, _tab_drafts_btn, _tab_media_btn, _tab_media_btn);
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
    if (_drafts_btn->isChecked())
        onShowDrafts();
    else
        onShowPosts();
}

void MainWindow::onMediaInsert(const WpMedia& media)
{
    if (_stack->currentWidget() == _editor)
        _editor->insertMedia(media);
    else
        _stack->setCurrentWidget(_editor);
}

} // namespace wpclient
