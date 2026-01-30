#pragma once

#include <memory>

#include <QString>
#include <QStringList>

class QListWidget;
class QListWidgetItem;

namespace database {
class IDatabaseManager;
}

class ClientListHelper {
public:
  explicit ClientListHelper(
      QListWidget *listWidget,
      std::shared_ptr<database::IDatabaseManager> dbManager);

  bool addUser(const QString &pseudonym);
  bool removeUser(const QString &pseudonym);
  void populateList(const QStringList &pseudonyms);

  void banUser(const QString &pseudonym);
  void unbanUser(const QString &pseudonym);
  bool isUserBanned(const QString &pseudonym) const;
  QString pseudonym(const QString &displayText) const;

private:
  QListWidgetItem *findItem(const QString &pseudonym) const;
  QString cleanPseudonym(const QString &text) const;
  void applyBannedDisplay(QListWidgetItem *item);
  void applyNormalDisplay(QListWidgetItem *item);

  QListWidget *listWidget_;
  std::shared_ptr<database::IDatabaseManager> dbManager_;
  const QString bannedSuffix_ = " - banned";
};