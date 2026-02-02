#include "ui/chat_window.hpp"

#include <optional>

#include <QAbstractItemView>
#include <QAction>
#include <QDateTime>
#include <QDebug>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <qglobal.h>
#include <qnamespace.h>
#include <qobject.h>
#include <utility>

#include "chat.grpc.pb.h"
#include "ui/client_list_helper.hpp"
#include "ui/private_chat_window.hpp"

ChatWindow::ChatWindow(std::shared_ptr<database::IDatabaseManager> dbManager,
                       QWidget *parent)
    : QWidget(parent), dbManager_(std::move(dbManager)) {
  setupUi();
}

ChatWindow::~ChatWindow() {
  stopMessageStream();
  stopClientEventStream();
}

void ChatWindow::prepareClose() {
  stopMessageStream();
  stopClientEventStream();

  if (!pseudonym_.isEmpty()) {
    emit disconnectRequested(pseudonym_);
  }

  connected_ = false;
}

void ChatWindow::setupUi() {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);

  auto *contentLayout = new QHBoxLayout();
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(8);

  conversation_ = new QTextBrowser(this);
  conversation_->setReadOnly(true);
  conversation_->setPlaceholderText("Conversation will appear here...");
  conversation_->setOpenExternalLinks(true);
  contentLayout->addWidget(conversation_, 3);

  clientsList_ = new QListWidget(this);
  clientsList_->setSelectionMode(QAbstractItemView::SingleSelection);
  clientsList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  clientsList_->setSortingEnabled(true);
  clientsList_->setUniformItemSizes(true);
  clientsList_->setToolTip("Connected chatters");
  clientsList_->setMinimumWidth(160);
  contentLayout->addWidget(clientsList_, 1);

  clientListHelper_ =
      std::make_unique<ClientListHelper>(clientsList_, dbManager_);

  layout->addLayout(contentLayout, 1);

  input_ = new QLineEdit(this);
  input_->setPlaceholderText("Type a message and press Enter...");

  sendButton_ = new QPushButton("Send", this);
  sendButton_->setDefault(true);

  auto *inputLayout = new QHBoxLayout();
  inputLayout->setContentsMargins(0, 0, 0, 0);
  inputLayout->addWidget(input_);
  inputLayout->addWidget(sendButton_);

  layout->addLayout(inputLayout);

  connect(sendButton_, &QPushButton::clicked, this, [this]() { handleSend(); });
  connect(input_, &QLineEdit::returnPressed, this, [this]() { handleSend(); });

  clientsList_->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(clientsList_, &QListWidget::customContextMenuRequested, this,
          &ChatWindow::showClientsContextMenu);

  clientsContextMenu_ = new QMenu(this);
  auto *privateMessageAction =
      clientsContextMenu_->addAction("Send private message");
  connect(privateMessageAction, &QAction::triggered, this, [this]() {
    auto *item = clientsList_->currentItem();
    if (item != nullptr) {
      openPrivateChatWith(clientListHelper_->pseudonym(item->text()));
    }
  });

  banUnbanAction_ = clientsContextMenu_->addAction("Ban/Unban");
  connect(banUnbanAction_, &QAction::triggered, this, [this]() {
    auto *item = clientsList_->currentItem();
    if (item != nullptr) {
      banUnbanUser(item);
    }
  });
}

void ChatWindow::addMessage(const QString &author, const QString &message,
                            QString color) {
  const auto timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  const auto formatted =
      QStringLiteral("<span style='color: %1;'> <b>%2</b> [%3]: %4 </span>")
          .arg(color, author, timestamp, message.toHtmlEscaped());
  conversation_->append(formatted);
}

void ChatWindow::addPrivateMessage(const QString &author,
                                   const QString &message) {
  const auto key = author.toLower();

  // manage private window in case of new private message in.
  PrivateChatWindow *window = nullptr;
  if (privateChats_.contains(key) && !privateChats_.value(key).isNull()) {
    window = privateChats_.value(key);
    if (!window->isVisible()) {
      window->show();
    }
  } else {
    window = new PrivateChatWindow(pseudonym_, author, this);

    connect(window, &QObject::destroyed, this,
            [this, key]() { privateChats_.remove(key); });

    connect(window, &PrivateChatWindow::sendPrivateMessageRequested, this,
            &ChatWindow::onPrivateMessageRequested);

    privateChats_.insert(key, window);
    window->show();
  }

  window->addMessage(author, message);
  window->raise();
  window->activateWindow();
}

void ChatWindow::handleSend() {
  if (!connected_) {
    QMessageBox::information(this, "Not connected",
                             "Connect to the server before sending messages.");
    return;
  }

  const auto text = input_->text().trimmed();
  if (text.isEmpty()) {
    return;
  }

  input_->clear();

  emit sendMessageRequested(text);
}

void ChatWindow::onLoginSucceeded(const QString &pseudonym,
                                  const QString &country,
                                  const QString &welcomeMessage,
                                  const QStringList &connectedPseudonyms) {
  pseudonym_ = pseudonym;
  country_ = country;
  connected_ = true;
  emit loginCompleted();
  startMessageStream();
  startClientEventStream();
  initChatView(welcomeMessage, connectedPseudonyms);
  qDebug() << "Successfull login for user '" << pseudonym << "'";
}

void ChatWindow::onDisconnectFinished(bool ok, const QString &errorText) {
  if (ok || errorText.isEmpty()) {
    return;
  }

  qWarning()
      << QStringLiteral("Failed to send disconnect frame: %1").arg(errorText);
}

void ChatWindow::onSendMessageFinished(bool ok, const QString &errorText) {
  if (ok || errorText.isEmpty()) {
    return;
  }

  QMessageBox::warning(this, "Send failed", errorText);
}

void ChatWindow::onMessageReceived(const QString &author,
                                   const QString &content, bool isPrivate) {
  if (isPrivate) {
    addPrivateMessage(author, content);
  } else {
    addMessage(author, content);
  }
}

void ChatWindow::onMessageStreamError(const QString &errorText) {
  if (errorText.isEmpty()) {
    return;
  }
  addMessage("System",
             QStringLiteral("Message stream stopped: %1").arg(errorText));
}

void ChatWindow::onClientEventReceived(int eventType,
                                       const QString &pseudonym) {
  handleClientEvent(eventType, pseudonym);
}

void ChatWindow::onClientEventStreamError(const QString &errorText) {
  if (errorText.isEmpty()) {
    return;
  }
  addMessage("System",
             QStringLiteral("Client event stream stopped: %1").arg(errorText));
}

void ChatWindow::initChatView(const QString &welcomeMessage,
                              const QStringList &connectedPseudonyms) {
  conversation_->clear();

  QStringList allUsers;
  allUsers.append(pseudonym_);
  allUsers.append(connectedPseudonyms);
  clientListHelper_->populateList(allUsers);

  if (clientsList_->count() > 0) {
    clientsList_->setCurrentRow(0);
  }

  addMessage(
      "System",
      QStringLiteral("Connected as %1 from %2").arg(pseudonym_, country_),
      MESSAGE_COLOR_SYSTEM_);

  if (!welcomeMessage.trimmed().isEmpty()) {
    addMessage("Server", welcomeMessage.trimmed(), MESSAGE_COLOR_SYSTEM_);
  }

  input_->setFocus();
}

void ChatWindow::handleClientEvent(int eventType, const QString &pseudonym) {
  if (pseudonym.isEmpty()) {
    return;
  }

  switch (eventType) {
  case chat::ClientEventData::ADD: {
    if (clientListHelper_->addUser(pseudonym)) {
      addMessage("System", QStringLiteral("%1 joined the chat.").arg(pseudonym),
                 MESSAGE_COLOR_USER_CONNECT_);
    }
    break;
  }
  case chat::ClientEventData::REMOVE: {
    if (clientListHelper_->removeUser(pseudonym)) {
      addMessage("System",
                 QStringLiteral("%1 has left the chat.").arg(pseudonym),
                 MESSAGE_COLOR_USER_DISCONNECT_);
    }
    break;
  }
  default:
    break;
  }
}

void ChatWindow::startMessageStream() { emit startMessageStreamRequested(); }

void ChatWindow::stopMessageStream() { emit stopMessageStreamRequested(); }

void ChatWindow::startClientEventStream() {
  emit startClientEventStreamRequested();
}

void ChatWindow::stopClientEventStream() {
  emit stopClientEventStreamRequested();
}

void ChatWindow::showClientsContextMenu(const QPoint &pos) {
  auto *item = clientsList_->itemAt(pos);
  if (item == nullptr) {
    return;
  }

  const auto pseudonym = clientListHelper_->pseudonym(item->text());

  if (QString::compare(pseudonym, pseudonym_, Qt::CaseInsensitive) == 0) {
    return;
  }

  clientsList_->setCurrentItem(item);
  clientsContextMenu_->popup(clientsList_->mapToGlobal(pos));
}

void ChatWindow::openPrivateChatWith(const QString &pseudonym) {
  const auto key = pseudonym.toLower();

  if (privateChats_.contains(key) && !privateChats_.value(key).isNull()) {
    PrivateChatWindow *window = privateChats_.value(key);
    window->raise();
    window->activateWindow();
    return;
  }

  auto *window = new PrivateChatWindow(pseudonym_, pseudonym, this);

  connect(window, &QObject::destroyed, this,
          [this, key]() { privateChats_.remove(key); });

  connect(window, &PrivateChatWindow::sendPrivateMessageRequested, this,
          &ChatWindow::onPrivateMessageRequested);

  privateChats_.insert(key, window);
  window->show();
}

void ChatWindow::onPrivateMessageRequested(const QString &recipient,
                                           const QString &content) {
  emit sendMessageRequested(content, std::make_optional(recipient));
}

void ChatWindow::banUnbanUser(QListWidgetItem *item) {
  const auto pseudonym = item->text();
  clientListHelper_->isUserBanned(pseudonym)
      ? clientListHelper_->unbanUser(pseudonym)
      : clientListHelper_->banUser(pseudonym);
}

void ChatWindow::onUserUnbanned(const QString &pseudonym) {
  if (clientListHelper_) {
    clientListHelper_->unbanUser(pseudonym);
  }
}
