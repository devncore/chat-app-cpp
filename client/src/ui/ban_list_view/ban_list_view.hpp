#pragma once

#include <QDockWidget>

namespace database {
class IDatabaseManager;
}

class BannedUsersModel;
class QListView;

class BanListView : public QDockWidget {
  Q_OBJECT

public:
  explicit BanListView(database::IDatabaseManager *dbManager,
                       QWidget *parent = nullptr);

  void refresh();

signals:
  void userUnbanned(const QString &pseudonym);

private:
  void showContextMenu(const QPoint &pos);

  BannedUsersModel *model_;
  QListView *listView_;
};
