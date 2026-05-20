#pragma once
#include "api/wp-models.hpp"

#include <vector>

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>

namespace wpclient {

    class SiteManager : public QDialog {
        Q_OBJECT

    public:
        explicit SiteManager(const std::vector<WpSite>& sites, QWidget* parent = nullptr);

        std::vector<WpSite> sites() const { return _sites; }

    private slots:
        void onAdd();
        void onRemove();
        void onTest();
        void onSelectionChanged();

    private:
        void populate();
        bool validateAndSave(QDialog* dlg, QLineEdit* url, QLineEdit* name,
                             QLineEdit* user, QLineEdit* pass, WpSite& out);

        std::vector<WpSite> _sites;
        QListWidget*        _list       = nullptr;
        QPushButton*        _add_btn    = nullptr;
        QPushButton*        _remove_btn = nullptr;
        QPushButton*        _test_btn   = nullptr;
        QLabel*             _hint_lbl   = nullptr;
    };

} // namespace wpclient
