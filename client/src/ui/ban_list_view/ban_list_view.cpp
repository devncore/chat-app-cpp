#include "ui/ban_list_view/ban_list_view.hpp"

#include <QListView>
#include <QMenu>

#include "ui/ban_list_view/banned_users_model.hpp"

BanListView::BanListView(database::IDatabaseManager *dbManager,
                          QWidget *parent)
    : QDockWidget("Banned Users", parent) {
  setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  setFeatures(QDockWidget::DockWidgetMovable |
              QDockWidget::DockWidgetClosable);

  model_ = new BannedUsersModel(dbManager, this);

  listView_ = new QListView(this);
  listView_->setModel(model_);
  listView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  listView_->setSelectionMode(QAbstractItemView::SingleSelection);
  listView_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(listView_, &QListView::customContextMenuRequested, this,
          &BanListView::showContextMenu);

  setWidget(listView_);
}

void BanListView::refresh() { model_->refresh(); }

void BanListView::showContextMenu(const QPoint &pos) {
  const auto index = listView_->indexAt(pos);
  if (!index.isValid()) {
    return;
  }

  const auto pseudonym = index.data(Qt::DisplayRole).toString();

  QMenu menu(this);
  auto *removeAction = menu.addAction("Remove");
  if (menu.exec(listView_->viewport()->mapToGlobal(pos)) == removeAction) {
    if (model_->removeUser(pseudonym)) {
      emit userUnbanned(pseudonym);
    }
  }
}
