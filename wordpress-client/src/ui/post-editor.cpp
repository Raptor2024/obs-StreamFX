#include "post-editor.hpp"
#include "media-picker.hpp"

#include <QDateTime>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPixmap>
#include <QScrollArea>
#include <QSplitter>
#include <QTextCursor>
#include <QTextCharFormat>
#include <QToolButton>
#include <QVBoxLayout>

namespace wpclient {

PostEditor::PostEditor(QWidget* parent) : QWidget(parent)
{
    auto* main_layout = new QVBoxLayout(this);
    main_layout->setContentsMargins(0, 0, 0, 0);

    // ---- Toolbar ----
    _toolbar = new QToolBar;
    auto* bold_act      = _toolbar->addAction("B",   this, &PostEditor::onBold);
    auto* italic_act    = _toolbar->addAction("I",   this, &PostEditor::onItalic);
    auto* underline_act = _toolbar->addAction("U",   this, &PostEditor::onUnderline);
    _toolbar->addSeparator();
    _toolbar->addAction("Link",  this, &PostEditor::onLink);
    _toolbar->addSeparator();
    _media_btn = new QPushButton("Insert Media");
    _media_btn->setObjectName("insert-media-btn");
    _toolbar->addWidget(_media_btn);

    QFont bf = bold_act->font();      bf.setBold(true);      bold_act->setFont(bf);
    QFont it = italic_act->font();    it.setItalic(true);    italic_act->setFont(it);
    QFont ul = underline_act->font(); ul.setUnderline(true); underline_act->setFont(ul);

    main_layout->addWidget(_toolbar);

    // ---- Title ----
    _title_le = new QLineEdit;
    _title_le->setObjectName("post-title");
    _title_le->setPlaceholderText("Post title");
    main_layout->addWidget(_title_le);

    // ---- Editor + sidebar splitter ----
    auto* splitter = new QSplitter(Qt::Horizontal);

    _content = new QTextEdit;
    _content->setAcceptRichText(true);
    _content->setPlaceholderText("Write your post here...");
    splitter->addWidget(_content);

    // ---- Right sidebar ----
    auto* sidebar    = new QWidget;
    auto* sidebar_vl = new QVBoxLayout(sidebar);
    sidebar->setFixedWidth(220);

    auto* status_grp  = new QGroupBox("Status");
    auto* status_fl   = new QVBoxLayout(status_grp);
    _status_combo     = new QComboBox;
    _status_combo->addItems({"draft", "publish", "pending", "future"});
    status_fl->addWidget(_status_combo);

    _date_edit = new QDateTimeEdit(QDateTime::currentDateTime());
    _date_edit->setDisplayFormat("yyyy-MM-dd HH:mm");
    _date_edit->setCalendarPopup(true);
    status_fl->addWidget(new QLabel("Publish date:"));
    status_fl->addWidget(_date_edit);

    auto* cat_grp  = new QGroupBox("Categories");
    auto* cat_fl   = new QVBoxLayout(cat_grp);
    _cat_list = new QListWidget;
    _cat_list->setSelectionMode(QAbstractItemView::MultiSelection);
    _cat_list->setMaximumHeight(120);
    cat_fl->addWidget(_cat_list);

    auto* tag_grp  = new QGroupBox("Tags");
    auto* tag_fl   = new QVBoxLayout(tag_grp);
    _tag_list = new QListWidget;
    _tag_list->setSelectionMode(QAbstractItemView::MultiSelection);
    _tag_list->setMaximumHeight(100);
    tag_fl->addWidget(_tag_list);

    sidebar_vl->addWidget(status_grp);
    sidebar_vl->addWidget(cat_grp);
    sidebar_vl->addWidget(tag_grp);
    sidebar_vl->addStretch();

    splitter->addWidget(sidebar);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    main_layout->addWidget(splitter, 1);

    // ---- Buttons ----
    auto* btn_row   = new QHBoxLayout;
    _draft_btn   = new QPushButton("Save Draft");
    _publish_btn = new QPushButton("Publish");
    _close_btn   = new QPushButton("Close");
    _draft_btn->setObjectName("draft-btn");
    _publish_btn->setObjectName("publish-btn");
    _close_btn->setObjectName("close-btn");
    _publish_btn->setDefault(true);
    btn_row->addWidget(_close_btn);
    btn_row->addStretch();
    btn_row->addWidget(_draft_btn);
    btn_row->addWidget(_publish_btn);
    main_layout->addLayout(btn_row);

    connect(_draft_btn,   &QPushButton::clicked, this, &PostEditor::onSaveDraft);
    connect(_publish_btn, &QPushButton::clicked, this, &PostEditor::onPublish);
    connect(_close_btn,   &QPushButton::clicked, this, &PostEditor::onClose);
    connect(_media_btn,   &QPushButton::clicked, this, &PostEditor::onInsertMediaClicked);
}

void PostEditor::setClient(WpClient* client)
{
    _client = client;
    populateTaxonomy();
}

void PostEditor::openNew()
{
    _is_new  = true;
    _current = {};
    _title_le->clear();
    _content->clear();
    _status_combo->setCurrentText("draft");
    _date_edit->setDateTime(QDateTime::currentDateTime());
    for (int i = 0; i < _cat_list->count(); ++i)
        _cat_list->item(i)->setSelected(false);
    for (int i = 0; i < _tag_list->count(); ++i)
        _tag_list->item(i)->setSelected(false);
}

void PostEditor::openPost(const WpPost& post)
{
    _is_new  = false;
    _current = post;
    _title_le->setText(QString::fromStdString(post.title));
    _content->setHtml(QString::fromStdString(post.content));
    _status_combo->setCurrentText(QString::fromStdString(post.status));

    QDateTime dt = QDateTime::fromString(QString::fromStdString(post.date), Qt::ISODate);
    if (dt.isValid())
        _date_edit->setDateTime(dt);

    // Restore category / tag selections
    for (int i = 0; i < _cat_list->count(); ++i) {
        int64_t id = _cat_list->item(i)->data(Qt::UserRole).toLongLong();
        bool sel = std::find(post.category_ids.begin(), post.category_ids.end(), id) != post.category_ids.end();
        _cat_list->item(i)->setSelected(sel);
    }
    for (int i = 0; i < _tag_list->count(); ++i) {
        int64_t id = _tag_list->item(i)->data(Qt::UserRole).toLongLong();
        bool sel = std::find(post.tag_ids.begin(), post.tag_ids.end(), id) != post.tag_ids.end();
        _tag_list->item(i)->setSelected(sel);
    }
}

void PostEditor::insertMedia(const WpMedia& media)
{
    QString url_str = QString::fromStdString(media.url);
    QString alt     = QString::fromStdString(media.alt_text);

    // Pre-fetch the image so QTextEdit can render it — Qt's QTextDocument
    // doesn't resolve http(s) resources on its own.
    auto* reply = _nam.get(QNetworkRequest(QUrl(url_str)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, url_str, alt]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QPixmap pm;
            if (pm.loadFromData(reply->readAll()))
                _content->document()->addResource(
                    QTextDocument::ImageResource, QUrl(url_str), QVariant(pm));
        }
        // Insert after resource is registered so loadResource() finds it immediately
        _content->textCursor().insertHtml(
            QString("<p><img src=\"%1\" alt=\"%2\" style=\"max-width:100%;height:auto;display:block;\" /></p>")
                .arg(url_str.toHtmlEscaped(), alt.toHtmlEscaped()));
    });
}

void PostEditor::populateTaxonomy()
{
    if (!_client)
        return;

    _client->fetchCategories([this](bool ok, std::vector<WpCategory> cats, ApiError) {
        if (!ok) return;
        _categories = cats;
        _cat_list->clear();
        for (const auto& c : cats) {
            auto* item = new QListWidgetItem(QString::fromStdString(c.name));
            item->setData(Qt::UserRole, static_cast<qlonglong>(c.id));
            _cat_list->addItem(item);
        }
    });

    _client->fetchTags([this](bool ok, std::vector<WpTag> tags, ApiError) {
        if (!ok) return;
        _tags = tags;
        _tag_list->clear();
        for (const auto& t : tags) {
            auto* item = new QListWidgetItem(QString::fromStdString(t.name));
            item->setData(Qt::UserRole, static_cast<qlonglong>(t.id));
            _tag_list->addItem(item);
        }
    });
}

void PostEditor::submitPost(const std::string& status)
{
    if (!_client)
        return;

    WpPost p     = _current;
    p.title      = _title_le->text().trimmed().toStdString();

    // QTextEdit::toHtml() wraps content in a full HTML document including a
    // <head><style> block with Qt's internal styles. WordPress would store
    // those CSS rules as visible text in the post. Extract only the body.
    {
        QString full = _content->toHtml();
        int start = full.indexOf("<body");
        if (start != -1) {
            start = full.indexOf('>', start) + 1;
            int end = full.lastIndexOf("</body>");
            full = (end != -1) ? full.mid(start, end - start).trimmed() : full;
        }
        p.content = full.toStdString();
    }
    p.status     = status;
    p.date       = _date_edit->dateTime().toString(Qt::ISODate).toStdString();

    p.category_ids.clear();
    for (auto* item : _cat_list->selectedItems())
        p.category_ids.push_back(item->data(Qt::UserRole).toLongLong());

    p.tag_ids.clear();
    for (auto* item : _tag_list->selectedItems())
        p.tag_ids.push_back(item->data(Qt::UserRole).toLongLong());

    if (p.title.empty()) {
        QMessageBox::warning(this, "Validation", "Title cannot be empty.");
        return;
    }

    auto cb = [this](bool ok, WpPost result, ApiError err) {
        if (!ok) {
            QMessageBox::critical(this, "Error", QString::fromStdString(err.message));
            return;
        }
        _current = result;
        _is_new  = false;
        emit saved(result);
    };

    if (_is_new)
        _client->createPost(p, cb);
    else
        _client->updatePost(p, cb);
}

void PostEditor::onBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(_content->fontWeight() == QFont::Bold ? QFont::Normal : QFont::Bold);
    _content->mergeCurrentCharFormat(fmt);
}

void PostEditor::onItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(!_content->fontItalic());
    _content->mergeCurrentCharFormat(fmt);
}

void PostEditor::onUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(!_content->fontUnderline());
    _content->mergeCurrentCharFormat(fmt);
}

void PostEditor::onLink()
{
    bool ok;
    QString url = QInputDialog::getText(this, "Insert Link", "URL:", QLineEdit::Normal, "https://", &ok);
    if (!ok || url.isEmpty())
        return;
    QString text = _content->textCursor().selectedText();
    if (text.isEmpty())
        text = url;
    _content->textCursor().insertHtml(
        QString("<a href=\"%1\">%2</a>").arg(url.toHtmlEscaped(), text.toHtmlEscaped()));
}

void PostEditor::onSaveDraft()
{
    submitPost("draft");
}

void PostEditor::onPublish()
{
    submitPost("publish");
}

void PostEditor::onClose()
{
    emit closed();
}

void PostEditor::onInsertMediaClicked()
{
    if (!_client)
        return;
    auto* picker = new MediaPicker(this);
    picker->setClient(_client);
    picker->setWindowFlags(Qt::Dialog);
    picker->setAttribute(Qt::WA_DeleteOnClose);
    connect(picker, &MediaPicker::mediaSelected, this, &PostEditor::insertMedia);
    connect(picker, &MediaPicker::mediaSelected, picker, &QWidget::close);
    picker->load();
    picker->show();
}

} // namespace wpclient
