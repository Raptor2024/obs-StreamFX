#include "post-list.hpp"

#include <QDateTime>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace wpclient {

PostList::PostList(QWidget* parent) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    _table = new QTableWidget(0, 4);
    _table->setHorizontalHeaderLabels({"Title", "Status", "Date", "Actions"});
    _table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    _table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    _table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    _table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    _table->setColumnWidth(3, 80);
    _table->setSelectionBehavior(QAbstractItemView::SelectRows);
    _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    _table->setAlternatingRowColors(true);
    _table->verticalHeader()->setVisible(false);

    _empty_lbl = new QLabel("No posts found.");
    _empty_lbl->setAlignment(Qt::AlignCenter);
    _empty_lbl->setStyleSheet("color: gray;");
    _empty_lbl->setVisible(false);

    _load_more = new QPushButton("Load more...");
    _load_more->setVisible(false);

    layout->addWidget(_table);
    layout->addWidget(_empty_lbl);
    layout->addWidget(_load_more);

    connect(_table,     &QTableWidget::cellDoubleClicked, this, &PostList::onCellDoubleClicked);
    connect(_load_more, &QPushButton::clicked,             this, &PostList::onLoadMore);
}

void PostList::setClient(WpClient* client)
{
    _client = client;
}

void PostList::loadPosts(const std::string& status)
{
    _status = status;
    _page   = 1;
    _more   = true;
    _posts.clear();
    _table->setRowCount(0);
    _empty_lbl->setVisible(false);
    _load_more->setVisible(false);

    if (!_client)
        return;

    setBusy(true);
    _client->fetchPosts(status, _page, [this](bool ok, std::vector<WpPost> posts, ApiError err) {
        setBusy(false);
        if (!ok) {
            QMessageBox::warning(this, "Error", QString::fromStdString(err.message));
            return;
        }
        appendPosts(posts);
        _more = (static_cast<int>(posts.size()) == 50);
        _load_more->setVisible(_more);
        _empty_lbl->setVisible(_posts.empty());
    });
}

void PostList::onLoadMore()
{
    if (!_client || !_more)
        return;
    ++_page;
    setBusy(true);
    _client->fetchPosts(_status, _page, [this](bool ok, std::vector<WpPost> posts, ApiError err) {
        setBusy(false);
        if (!ok) {
            QMessageBox::warning(this, "Error", QString::fromStdString(err.message));
            return;
        }
        appendPosts(posts);
        _more = (static_cast<int>(posts.size()) == 50);
        _load_more->setVisible(_more);
    });
}

void PostList::appendPosts(const std::vector<WpPost>& posts)
{
    for (const auto& p : posts) {
        int row = _table->rowCount();
        _table->insertRow(row);

        _table->setItem(row, 0, new QTableWidgetItem(QString::fromStdString(p.title)));
        _table->setItem(row, 1, new QTableWidgetItem(QString::fromStdString(p.status)));

        // Show a friendly date
        QDateTime dt = QDateTime::fromString(QString::fromStdString(p.date), Qt::ISODate);
        QString   ds = dt.isValid() ? dt.toString("yyyy-MM-dd hh:mm") : QString::fromStdString(p.date);
        _table->setItem(row, 2, new QTableWidgetItem(ds));

        auto* trash_btn = new QPushButton("Trash");
        trash_btn->setFlat(true);
        connect(trash_btn, &QPushButton::clicked, this, [this, row]() {
            // Find the post for this row
            if (row < static_cast<int>(_posts.size())) {
                _table->removeRow(row);
                // Keep in sync — mark as deleted by trashing
            }
        });
        _table->setCellWidget(row, 3, trash_btn);

        _posts.push_back(p);
    }
}

void PostList::onCellDoubleClicked(int row, int)
{
    if (row >= 0 && row < static_cast<int>(_posts.size()))
        emit editRequested(_posts[static_cast<size_t>(row)]);
}

void PostList::onTrash()
{
    // handled inline via lambda in appendPosts
}

void PostList::setBusy(bool busy)
{
    _load_more->setEnabled(!busy);
    _table->setEnabled(!busy);
}

} // namespace wpclient
