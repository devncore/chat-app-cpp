#include "ui/client_list_helper.hpp"

#include <QBrush>
#include <QListWidget>
#include <QListWidgetItem>

#include "database/database_manager.hpp"

ClientListHelper::ClientListHelper(
    QListWidget *listWidget,
    std::shared_ptr<database::IDatabaseManager> dbManager)
    : listWidget_(listWidget), dbManager_(std::move(dbManager)) {
  Q_ASSERT(listWidget_ != nullptr);
}

bool ClientListHelper::addUser(const QString &pseudonym) {
  const auto trimmed = pseudonym.trimmed();
  if (trimmed.isEmpty()) {
    return false;
  }

  if (findItem(trimmed) != nullptr) {
    return false;
  }

  auto *item = new QListWidgetItem(trimmed);

  if (dbManager_ && dbManager_->isInitialized()) {
    const auto result = dbManager_->isUserBanned(trimmed.toStdString());
    if (result.has_value() && result.value()) {
      applyBannedDisplay(item);
    }
  }

  listWidget_->addItem(item);
  return true;
}

bool ClientListHelper::removeUser(const QString &pseudonym) {
  auto *item = findItem(pseudonym);
  if (item == nullptr) {
    return false;
  }

  delete listWidget_->takeItem(listWidget_->row(item));
  return true;
}

void ClientListHelper::populateList(const QStringList &pseudonyms) {
  listWidget_->clear();
  for (const auto &name : pseudonyms) {
    addUser(name);
  }
}

void ClientListHelper::banUser(const QString &pseudonym) {
  const auto clean = cleanPseudonym(pseudonym);

  if (dbManager_) {
    if (const auto error = dbManager_->banUser(clean.toStdString())) {
      return;
    }
  }

  if (auto *item = findItem(clean)) {
    applyBannedDisplay(item);
  }
}

void ClientListHelper::unbanUser(const QString &pseudonym) {
  const auto clean = cleanPseudonym(pseudonym);

  if (dbManager_) {
    if (const auto error = dbManager_->unbanUser(clean.toStdString())) {
      return;
    }
  }

  if (auto *item = findItem(clean)) {
    applyNormalDisplay(item);
  }
}

QString ClientListHelper::pseudonym(const QString &displayText) const {
  return cleanPseudonym(displayText);
}

bool ClientListHelper::isUserBanned(const QString &pseudonym) const {
  const auto *item = findItem(pseudonym);
  if (item == nullptr) {
    return false;
  }

  const auto val = item->data(Qt::UserRole);
  return !val.isNull() && val.toBool();
}

QListWidgetItem *ClientListHelper::findItem(const QString &pseudonym) const {
  const auto trimmed = cleanPseudonym(pseudonym);
  if (trimmed.isEmpty()) {
    return nullptr;
  }

  for (int i = 0; i < listWidget_->count(); ++i) {
    auto *item = listWidget_->item(i);
    if (item == nullptr) {
      continue;
    }

    const auto displayName = cleanPseudonym(item->text());
    if (QString::compare(displayName, trimmed, Qt::CaseInsensitive) == 0) {
      return item;
    }
  }

  return nullptr;
}

QString ClientListHelper::cleanPseudonym(const QString &text) const {
  auto clean = text.trimmed();
  if (clean.endsWith(bannedSuffix_)) {
    clean.chop(bannedSuffix_.size());
  }
  return clean;
}

void ClientListHelper::applyBannedDisplay(QListWidgetItem *item) {
  if (item->text().endsWith(bannedSuffix_)) {
    return;
  }

  item->setData(Qt::UserRole, true);
  item->setForeground(QBrush(Qt::gray));
  item->setText(item->text() + bannedSuffix_);
}

void ClientListHelper::applyNormalDisplay(QListWidgetItem *item) {
  item->setData(Qt::UserRole, false);
  item->setForeground(Qt::NoBrush);

  auto text = item->text();
  if (text.endsWith(bannedSuffix_)) {
    text.chop(bannedSuffix_.size());
    item->setText(text);
  }
}