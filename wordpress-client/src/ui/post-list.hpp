#pragma once
#include "api/wp-client.hpp"
#include "api/wp-models.hpp"

#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QWidget>

namespace wpclient {

    class PostList : public QWidget {
        Q_OBJECT

    public:
        explicit PostList(QWidget* parent = nullptr);

        void setClient(WpClient* client);
        void loadPosts(const std::string& status);

    signals:
        void editRequested(const WpPost& post);

    private slots:
        void onLoadMore();
        void onCellDoubleClicked(int row, int col);
        void onTrash();

    private:
        void appendPosts(const std::vector<WpPost>& posts);
        void setBusy(bool busy);

        WpClient*    _client     = nullptr;
        std::string  _status;
        int          _page       = 1;
        bool         _more       = true;

        QTableWidget* _table     = nullptr;
        QPushButton*  _load_more = nullptr;
        QLabel*       _empty_lbl = nullptr;

        std::vector<WpPost> _posts; // mirror for row → post mapping
    };

} // namespace wpclient
