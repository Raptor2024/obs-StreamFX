#include "site-manager.hpp"
#include "api/wp-auth.hpp"
#include "api/wp-client.hpp"

#include <QDialogButtonBox>
#include <QEventLoop>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QProgressDialog>
#include <QUuid>
#include <QVBoxLayout>

namespace wpclient {

SiteManager::SiteManager(const std::vector<WpSite>& sites, QWidget* parent)
    : QDialog(parent), _sites(sites)
{
    setWindowTitle("Manage Sites");
    setMinimumWidth(420);

    auto* layout = new QVBoxLayout(this);

    _hint_lbl = new QLabel(
        "Use <b>WordPress Application Passwords</b> (WP Admin → Users → Profile → "
        "Application Passwords) — never your main WP password.");
    _hint_lbl->setWordWrap(true);
    _hint_lbl->setStyleSheet("color: gray; font-size: 11px;");
    layout->addWidget(_hint_lbl);

    _list = new QListWidget;
    layout->addWidget(_list);

    auto* btn_row = new QHBoxLayout;
    _add_btn    = new QPushButton("Add Site");
    _remove_btn = new QPushButton("Remove");
    _test_btn   = new QPushButton("Test Connection");
    _remove_btn->setEnabled(false);
    _test_btn->setEnabled(false);
    btn_row->addWidget(_add_btn);
    btn_row->addWidget(_remove_btn);
    btn_row->addStretch();
    btn_row->addWidget(_test_btn);
    layout->addLayout(btn_row);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(_add_btn,    &QPushButton::clicked, this, &SiteManager::onAdd);
    connect(_remove_btn, &QPushButton::clicked, this, &SiteManager::onRemove);
    connect(_test_btn,   &QPushButton::clicked, this, &SiteManager::onTest);
    connect(_list, &QListWidget::itemSelectionChanged, this, &SiteManager::onSelectionChanged);

    populate();
}

void SiteManager::populate()
{
    _list->clear();
    for (const auto& s : _sites) {
        auto* item = new QListWidgetItem(
            QString("%1 (%2)").arg(
                QString::fromStdString(s.name),
                QString::fromStdString(s.url)));
        item->setData(Qt::UserRole, QString::fromStdString(s.id));
        _list->addItem(item);
    }
}

void SiteManager::onSelectionChanged()
{
    bool sel = !_list->selectedItems().isEmpty();
    _remove_btn->setEnabled(sel);
    _test_btn->setEnabled(sel);
}

void SiteManager::onAdd()
{
    QDialog dlg(this);
    dlg.setWindowTitle("Add WordPress Site");

    auto* form     = new QFormLayout;
    auto* name_le  = new QLineEdit;
    auto* url_le   = new QLineEdit;
    url_le->setPlaceholderText("https://myblog.com");
    auto* user_le  = new QLineEdit;
    auto* pass_le  = new QLineEdit;
    pass_le->setEchoMode(QLineEdit::Password);
    pass_le->setPlaceholderText("xxxx xxxx xxxx xxxx xxxx xxxx");

    form->addRow("Site name:",            name_le);
    form->addRow("Site URL:",             url_le);
    form->addRow("WordPress username:",   user_le);
    form->addRow("Application password:", pass_le);

    auto* warn = new QLabel("Only HTTPS sites are recommended.");
    warn->setStyleSheet("color: orange;");

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    auto* vl = new QVBoxLayout(&dlg);
    vl->addLayout(form);
    vl->addWidget(warn);
    vl->addWidget(buttons);

    connect(buttons, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    connect(url_le, &QLineEdit::textChanged, [warn](const QString& t) {
        warn->setVisible(!t.startsWith("https://"));
    });
    warn->setVisible(false);

    if (dlg.exec() != QDialog::Accepted)
        return;

    WpSite site;
    if (!validateAndSave(&dlg, url_le, name_le, user_le, pass_le, site))
        return;

    _sites.push_back(site);
    populate();
}

bool SiteManager::validateAndSave(QDialog*, QLineEdit* url, QLineEdit* name,
                                   QLineEdit* user, QLineEdit* pass, WpSite& out)
{
    QString u = url->text().trimmed();
    QString n = name->text().trimmed();
    QString usr = user->text().trimmed();
    QString pwd = pass->text().trimmed();

    if (n.isEmpty() || u.isEmpty() || usr.isEmpty() || pwd.isEmpty()) {
        QMessageBox::warning(this, "Validation", "All fields are required.");
        return false;
    }
    if (!u.startsWith("http://") && !u.startsWith("https://")) {
        QMessageBox::warning(this, "Validation", "URL must start with http:// or https://");
        return false;
    }

    out.id       = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    out.name     = n.toStdString();
    out.url      = u.toStdString();
    out.username = usr.toStdString();

    WpAuth::store(out.id, out.username, pwd.toStdString());

    // Overwrite the password field in memory
    pwd.fill(QChar('*'));
    pass->clear();

    return true;
}

void SiteManager::onRemove()
{
    auto* item = _list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto it = std::find_if(_sites.begin(), _sites.end(), [&id](const WpSite& s) {
        return s.id == id.toStdString();
    });
    if (it == _sites.end())
        return;

    if (QMessageBox::question(this, "Remove Site",
            QString("Remove \"%1\"?").arg(QString::fromStdString(it->name)))
        != QMessageBox::Yes)
        return;

    WpAuth::remove(it->id);
    _sites.erase(it);
    populate();
}

void SiteManager::onTest()
{
    auto* item = _list->currentItem();
    if (!item)
        return;

    QString id = item->data(Qt::UserRole).toString();
    auto it = std::find_if(_sites.begin(), _sites.end(), [&id](const WpSite& s) {
        return s.id == id.toStdString();
    });
    if (it == _sites.end())
        return;

    auto* prog = new QProgressDialog("Testing connection...", QString(), 0, 0, this);
    prog->setWindowModality(Qt::WindowModal);
    prog->show();

    WpClient* client = new WpClient(*it, this);
    client->testConnection([prog, client, this](bool ok, std::string name, ApiError err) {
        prog->close();
        prog->deleteLater();
        client->deleteLater();

        if (ok) {
            QMessageBox::information(this, "Connection OK",
                QString("Connected as: <b>%1</b>").arg(QString::fromStdString(name)));
        } else {
            QMessageBox::critical(this, "Connection Failed",
                QString("Error: %1").arg(QString::fromStdString(err.message)));
        }
    });
}

} // namespace wpclient
