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
#include <QMetaObject>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QVBoxLayout>

#include <utility>

#include "chat.grpc.pb.h"
#include "chat_client_session.hpp"

ChatWindow::ChatWindow(QString serverAddress,
                       std::unique_ptr<ChatClientSession> chatSession,
                       QWidget *parent)
    : QWidget(parent), stacked_(new QStackedWidget(this)),
      serverAddress_(std::move(serverAddress)),
      chatSession_(std::move(chatSession)) {
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

  if (chatSession_) {
    stopMessageStream();
    stopClientEventStream();

    if (!pseudonym.isEmpty()) {
      const auto status = chatSession_->disconnect(pseudonym.toStdString());
      if (!status.ok()) {
        qWarning() << QStringLiteral(
                          "Failed to send disconnect frame for %1: %2")
                          .arg(pseudonym,
                               QString::fromStdString(status.error_message()));
      }
    }
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

  if (!chatSession_) {
    QMessageBox::warning(this, "Client not ready",
                         "The gRPC client is not ready yet. Try reconnecting.");
    return;
  }

  const auto status = chatSession_->sendMessage(text.toStdString());
  if (!status.ok()) {
    QMessageBox::warning(this, "Send failed",
                         QString::fromStdString(status.error_message()));
  }
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

  if (!chatSession_) {
    QMessageBox::warning(
        this, "Client not ready",
        "The gRPC client is not ready yet. Try restarting the application.");
    return;
  }

  connectButton_->setEnabled(false);

  const auto result = chatSession_->connect(
      pseudonym.toStdString(), gender.toStdString(), country.toStdString());

  connectButton_->setEnabled(true);

  if (!result.status.ok()) {
    QMessageBox::critical(
        this, "Connection failed",
        QStringLiteral("Unable to connect to %1.\n%2").arg(serverAddress_)
            .arg(QString::fromStdString(result.status.error_message())));
    return;
  }

  if (!result.response.accepted()) {
    QMessageBox::warning(this, "Connection rejected",
                         QString::fromStdString(result.response.message()));
    return;
  }

  connected_ = true;
  setWindowTitle(QStringLiteral("Chat Client - %1").arg(pseudonym));
  switchToChatView(QString::fromStdString(result.response.message()));
  startMessageStream();
  startClientEventStream();
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
             MESSAGE_COLOR_SYSTEM);

  if (!welcomeMessage.trimmed().isEmpty()) {
    addMessage("Server", welcomeMessage.trimmed(), MESSAGE_COLOR_SYSTEM);
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

void ChatWindow::handleClientEvent(const chat::ClientEventData &eventData) {
  if (!clientsList_) {
    return;
  }

  QStringList names;
  names.reserve(eventData.pseudonyms_size());
  for (const auto &entry : eventData.pseudonyms()) {
    const auto name = QString::fromStdString(entry).trimmed();
    if (!name.isEmpty()) {
      names.append(name);
    }
  }

  if (names.isEmpty()) {
    return;
  }

  switch (eventData.event_type()) {
  case chat::ClientEventData::ADD: {
    for (const auto &name : names) {
      if (addClientToList(name)) {
        addMessage("System", QStringLiteral("%1 joined the chat.").arg(name),
                   MESSAGE_COLOR_USER_CONNECT);
      }
    }
    break;
  }
  case chat::ClientEventData::REMOVE: {
    for (const auto &name : names) {
      if (removeClientFromList(name)) {
        addMessage("System", QStringLiteral("%1 has left the chat.").arg(name),
                   MESSAGE_COLOR_USER_DISCONNECT);
      }
    }
    break;
  }
  case chat::ClientEventData::SYNC: {
    const auto previousSelection = clientsList_->currentItem()
                                       ? clientsList_->currentItem()->text()
                                       : QString{};
    clientsList_->clear();
    for (const auto &name : names) {
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
  if (!chatSession_) {
    return;
  }

  stopMessageStream();

  chatSession_->startMessageStream(
      [this](const chat::InformClientsNewMessageResponse &incoming) {
        const auto author = QString::fromStdString(incoming.author());
        const auto content = QString::fromStdString(incoming.content());
        QMetaObject::invokeMethod(
            this, [this, author, content]() { addMessage(author, content); },
            Qt::QueuedConnection);
      },
      [this](const std::string &errorText) {
        if (errorText.empty()) {
          return;
        }
        const auto formatted = QString::fromStdString(errorText);
        QMetaObject::invokeMethod(
            this,
            [this, formatted]() {
              addMessage(
                  "System",
                  QStringLiteral("Message stream stopped: %1").arg(formatted));
            },
            Qt::QueuedConnection);
      });
}

void ChatWindow::stopMessageStream() {
  if (chatSession_) {
    chatSession_->stopMessageStream();
  }
}

void ChatWindow::startClientEventStream() {
  if (!chatSession_) {
    return;
  }

  stopClientEventStream();

  chatSession_->startClientEventStream(
      [this](const chat::ClientEventData &incoming) {
        const auto eventData = incoming;
        QMetaObject::invokeMethod(
            this, [this, eventData]() { handleClientEvent(eventData); },
            Qt::QueuedConnection);
      },
      [this](const std::string &errorText) {
        if (errorText.empty()) {
          return;
        }
        const auto formatted = QString::fromStdString(errorText);
        QMetaObject::invokeMethod(
            this,
            [this, formatted]() {
              addMessage("System",
                         QStringLiteral("Client event stream stopped: %1")
                             .arg(formatted));
            },
            Qt::QueuedConnection);
      });
}

void ChatWindow::stopClientEventStream() {
  if (chatSession_) {
    chatSession_->stopClientEventStream();
  }
}
