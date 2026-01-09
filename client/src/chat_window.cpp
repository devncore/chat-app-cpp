#include "chat_window.hpp"

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QDebug>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <utility>

#include "chat.grpc.pb.h"
ChatWindow::ChatWindow(QString serverAddress, QWidget *parent)
    : QWidget(parent), stacked_(new QStackedWidget(this)),
      serverAddress_(std::move(serverAddress)) {
  setWindowTitle("Chat Client");
  resize(480, 480);

  loginView_ = createLoginView();
  chatView_ = createChatView();

  stacked_->addWidget(loginView_);
  stacked_->addWidget(chatView_);
  stacked_->setCurrentWidget(loginView_);

  auto *layout = new QVBoxLayout(this);
  layout->addWidget(stacked_);
}

ChatWindow::~ChatWindow() {
  stopMessageStream();
  stopClientEventStream();
}

void ChatWindow::closeEvent(QCloseEvent *event) {
  const QString pseudonym =
      pseudonymInput_ ? pseudonymInput_->text().trimmed() : QString{};

  stopMessageStream();
  stopClientEventStream();

  if (!pseudonym.isEmpty()) {
    emit disconnectRequested(pseudonym);
  }

  connected_ = false;

  QWidget::closeEvent(event);
}

QWidget *ChatWindow::createLoginView() {
  auto *widget = new QWidget(this);
  auto *layout = new QVBoxLayout(widget);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);

  auto *formLayout = new QFormLayout();
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->setFormAlignment(Qt::AlignTop);

  pseudonymInput_ = new QLineEdit(widget);
  pseudonymInput_->setText("John");
  formLayout->addRow("Pseudonym:", pseudonymInput_);

  genderInput_ = new QComboBox(widget);
  genderInput_->addItem("Male");
  genderInput_->addItem("Female");
  genderInput_->setCurrentText("Male");
  formLayout->addRow("Gender:", genderInput_);

  countryInput_ = new QLineEdit(widget);
  countryInput_->setText("France");
  formLayout->addRow("Country:", countryInput_);

  layout->addLayout(formLayout);

  connectButton_ = new QPushButton("Connect", widget);
  connectButton_->setDefault(true);
  layout->addWidget(connectButton_);

  layout->addStretch();

  connect(connectButton_, &QPushButton::clicked, this,
          [this]() { handleConnect(); });

  return widget;
}

QWidget *ChatWindow::createChatView() {
  auto *widget = new QWidget(this);
  auto *layout = new QVBoxLayout(widget);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);

  auto *contentLayout = new QHBoxLayout();
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(8);

  conversation_ = new QTextBrowser(widget);
  conversation_->setReadOnly(true);
  conversation_->setPlaceholderText("Conversation will appear here...");
  conversation_->setOpenExternalLinks(true);
  contentLayout->addWidget(conversation_, 3);

  clientsList_ = new QListWidget(widget);
  clientsList_->setSelectionMode(QAbstractItemView::SingleSelection);
  clientsList_->setEditTriggers(QAbstractItemView::NoEditTriggers);
  clientsList_->setSortingEnabled(true);
  clientsList_->setUniformItemSizes(true);
  clientsList_->setToolTip("Connected chatters");
  clientsList_->setMinimumWidth(160);
  contentLayout->addWidget(clientsList_, 1);

  layout->addLayout(contentLayout, 1);

  input_ = new QLineEdit(widget);
  input_->setPlaceholderText("Type a message and press Enter...");

  sendButton_ = new QPushButton("Send", widget);
  sendButton_->setDefault(true);

  auto *inputLayout = new QHBoxLayout();
  inputLayout->setContentsMargins(0, 0, 0, 0);
  inputLayout->addWidget(input_);
  inputLayout->addWidget(sendButton_);

  layout->addLayout(inputLayout);

  connect(sendButton_, &QPushButton::clicked, this, [this]() { handleSend(); });
  connect(input_, &QLineEdit::returnPressed, this, [this]() { handleSend(); });
  connect(clientsList_, &QListWidget::itemClicked, this,
          [this](QListWidgetItem *item) {
            if (!item) {
              return;
            }
            addMessage("System",
                       QStringLiteral("Selected %1 (private chat TBD).")
                           .arg(item->text()));
          });

  return widget;
}

void ChatWindow::addMessage(const QString &author, const QString &message,
                            QString color) {
  const auto timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  const auto formatted =
      QStringLiteral("<span style='color: %1;'> <b>%2</b> [%3]: %4 </span>")
          .arg(color, author, timestamp, message.toHtmlEscaped());
  conversation_->append(formatted);
}

void ChatWindow::handleSend() {
  if (!connected_) {
    QMessageBox::information(this, "Not connected",
                             "Connect to the server before sending messages.");
    return;
  }

  const auto text = input_->text().trimmed();
  if (text.isEmpty())
    return;

  input_->clear();

  emit sendMessageRequested(text);
}

void ChatWindow::handleConnect() {
  const auto pseudonym = pseudonymInput_->text().trimmed();
  const auto gender = genderInput_->currentText().trimmed();
  const auto country = countryInput_->text().trimmed();

  if (pseudonym.isEmpty() || gender.isEmpty() || country.isEmpty()) {
    QMessageBox::warning(
        this, "Incomplete details",
        "Please provide pseudonym, gender, and country before connecting.");
    return;
  }

  connectButton_->setEnabled(false);

  emit connectRequested(pseudonym, gender, country);
}

void ChatWindow::onConnectFinished(bool ok, const QString &errorText,
                                   bool accepted, const QString &message) {
  connectButton_->setEnabled(true);

  if (!ok) {
    QMessageBox::critical(
        this, "Connection failed",
        QStringLiteral("Unable to connect to %1.\n%2").arg(serverAddress_)
            .arg(errorText));
    return;
  }

  if (!accepted) {
    QMessageBox::warning(this, "Connection rejected", message);
    return;
  }

  connected_ = true;
  setWindowTitle(QStringLiteral("Chat Client - %1")
                     .arg(pseudonymInput_->text().trimmed()));
  switchToChatView(message);
  startMessageStream();
  startClientEventStream();
}

void ChatWindow::onDisconnectFinished(bool ok, const QString &errorText) {
  if (ok || errorText.isEmpty()) {
    return;
  }

  qWarning() << QStringLiteral("Failed to send disconnect frame: %1")
                    .arg(errorText);
}

void ChatWindow::onSendMessageFinished(bool ok, const QString &errorText) {
  if (ok || errorText.isEmpty()) {
    return;
  }

  QMessageBox::warning(this, "Send failed", errorText);
}

void ChatWindow::onMessageReceived(const QString &author,
                                   const QString &content) {
  addMessage(author, content);
}

void ChatWindow::onMessageStreamError(const QString &errorText) {
  if (errorText.isEmpty()) {
    return;
  }
  addMessage("System",
             QStringLiteral("Message stream stopped: %1").arg(errorText));
}

void ChatWindow::onClientEventReceived(int eventType,
                                       const QStringList &pseudonyms) {
  handleClientEvent(eventType, pseudonyms);
}

void ChatWindow::onClientEventStreamError(const QString &errorText) {
  if (errorText.isEmpty()) {
    return;
  }
  addMessage("System",
             QStringLiteral("Client event stream stopped: %1").arg(errorText));
}

void ChatWindow::switchToChatView(const QString &welcomeMessage) {
  stacked_->setCurrentWidget(chatView_);
  conversation_->clear();
  if (clientsList_) {
    clientsList_->clear();
    addClientToList(pseudonymInput_->text().trimmed());
    if (clientsList_->count() > 0) {
      clientsList_->setCurrentRow(0);
    }
  }

  addMessage("System",
             QStringLiteral("Connected as %1 from %2")
                 .arg(pseudonymInput_->text().trimmed(),
                      countryInput_->text().trimmed()),
             MESSAGE_COLOR_SYSTEM_);

  if (!welcomeMessage.trimmed().isEmpty()) {
    addMessage("Server", welcomeMessage.trimmed(), MESSAGE_COLOR_SYSTEM_);
  }

  input_->setFocus();
}

bool ChatWindow::addClientToList(const QString &pseudonym) {
  if (!clientsList_) {
    return false;
  }

  const auto trimmed = pseudonym.trimmed();
  if (trimmed.isEmpty()) {
    return false;
  }

  for (int i = 0; i < clientsList_->count(); ++i) {
    auto *item = clientsList_->item(i);
    if (item &&
        QString::compare(item->text(), trimmed, Qt::CaseInsensitive) == 0) {
      item->setText(trimmed);
      return false;
    }
  }

  clientsList_->addItem(trimmed);
  return true;
}

bool ChatWindow::removeClientFromList(const QString &pseudonym) {
  if (!clientsList_) {
    return false;
  }

  const auto trimmed = pseudonym.trimmed();
  if (trimmed.isEmpty()) {
    return false;
  }

  for (int i = 0; i < clientsList_->count(); ++i) {
    auto *item = clientsList_->item(i);
    if (item &&
        QString::compare(item->text(), trimmed, Qt::CaseInsensitive) == 0) {
      delete clientsList_->takeItem(i);
      return true;
    }
  }

  return false;
}

void ChatWindow::handleClientEvent(int eventType,
                                   const QStringList &pseudonyms) {
  if (!clientsList_) {
    return;
  }

  if (pseudonyms.isEmpty()) {
    return;
  }

  switch (eventType) {
  case chat::ClientEventData::ADD: {
    for (const auto &name : pseudonyms) {
      if (addClientToList(name)) {
        addMessage("System", QStringLiteral("%1 joined the chat.").arg(name),
                   MESSAGE_COLOR_USER_CONNECT_);
      }
    }
    break;
  }
  case chat::ClientEventData::REMOVE: {
    for (const auto &name : pseudonyms) {
      if (removeClientFromList(name)) {
        addMessage("System", QStringLiteral("%1 has left the chat.").arg(name),
                   MESSAGE_COLOR_USER_DISCONNECT_);
      }
    }
    break;
  }
  case chat::ClientEventData::SYNC: {
    const auto previousSelection = clientsList_->currentItem()
                                       ? clientsList_->currentItem()->text()
                                       : QString{};
    clientsList_->clear();
    for (const auto &name : pseudonyms) {
      addClientToList(name);
    }

    if (!previousSelection.isEmpty()) {
      const auto matches = clientsList_->findItems(
          previousSelection, Qt::MatchFixedString | Qt::MatchCaseSensitive);
      if (!matches.isEmpty()) {
        clientsList_->setCurrentItem(matches.first());
      }
    }
    break;
  }
  default:
    break;
  }
}

void ChatWindow::startMessageStream() {
  emit startMessageStreamRequested();
}

void ChatWindow::stopMessageStream() {
  emit stopMessageStreamRequested();
}

void ChatWindow::startClientEventStream() {
  emit startClientEventStreamRequested();
}

void ChatWindow::stopClientEventStream() {
  emit stopClientEventStreamRequested();
}
