#include "ui/ban_list_view/banned_users_model.hpp"

#include <QDebug>

#include "database/database_manager.hpp"

BannedUsersModel::BannedUsersModel(database::IDatabaseManager *dbManager,
                                   QObject *parent)
    : QAbstractListModel(parent), dbManager_(dbManager) {}

int BannedUsersModel::rowCount(const QModelIndex &parent) const {
  if (parent.isValid()) {
    return 0;
  }
  return static_cast<int>(pseudonyms_.size());
}

QVariant BannedUsersModel::data(const QModelIndex &index, int role) const {
  if (!index.isValid() || index.row() < 0 ||
      index.row() >= pseudonyms_.size()) {
    return {};
  }

  if (role == Qt::DisplayRole) {
    return pseudonyms_.at(index.row());
  }

  return {};
}

bool BannedUsersModel::removeUser(const QString &pseudonym) {
  if (dbManager_ == nullptr || !dbManager_->isInitialized()) {
    return false;
  }

  if (const auto error = dbManager_->unbanUser(pseudonym.toStdString())) {
    qDebug() << "Failed to unban user:" << error->c_str();
    return false;
  }

  const int idx = static_cast<int>(pseudonyms_.indexOf(pseudonym));
  if (idx < 0) {
    return true;
  }

  beginRemoveRows(QModelIndex(), idx, idx);
  pseudonyms_.removeAt(idx);
  endRemoveRows();
  return true;
}

void BannedUsersModel::refresh() {
  if (dbManager_ == nullptr || !dbManager_->isInitialized()) {
    beginResetModel();
    pseudonyms_.clear();
    endResetModel();
    return;
  }

  auto result = dbManager_->getAllBannedUsers();
  if (!result.has_value()) {
    qDebug() << "Failed to load banned users:" << result.error().c_str();
    return;
  }

  beginResetModel();
  pseudonyms_.clear();
  for (const auto &pseudonym : result.value()) {
    pseudonyms_.append(QString::fromStdString(pseudonym));
  }
  pseudonyms_.sort(Qt::CaseInsensitive);
  endResetModel();
}
