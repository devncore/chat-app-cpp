#include "chat_window.h"

#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QStackedWidget>
#include <QString>
#include <QTextBrowser>
#include <QVBoxLayout>

#include "chat.grpc.pb.h"
#include "grpc_chat_client.h"

namespace {
constexpr char kServerAddress[] = "localhost:50051";
}

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent), stacked_(new QStackedWidget(this)) {
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

ChatWindow::~ChatWindow() { stopMessageStream(); }

QWidget *ChatWindow::createLoginView() {
  auto *widget = new QWidget(this);
  auto *layout = new QVBoxLayout(widget);
  layout->setContentsMargins(24, 24, 24, 24);
  layout->setSpacing(16);

  auto *formLayout = new QFormLayout();
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->setFormAlignment(Qt::AlignTop);

  pseudonymInput_ = new QLineEdit(widget);
  pseudonymInput_->setPlaceholderText("John");
  formLayout->addRow("Pseudonym:", pseudonymInput_);

  genderInput_ = new QLineEdit(widget);
  genderInput_->setPlaceholderText("Male");
  formLayout->addRow("Gender:", genderInput_);

  countryInput_ = new QLineEdit(widget);
  countryInput_->setPlaceholderText("France");
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

  conversation_ = new QTextBrowser(widget);
  conversation_->setReadOnly(true);
  conversation_->setPlaceholderText("Conversation will appear here...");
  conversation_->setOpenExternalLinks(true);

  layout->addWidget(conversation_, 1);

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

  return widget;
}

void ChatWindow::addMessage(const QString &author, const QString &message) {
  const auto timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  const auto formatted = QStringLiteral("<b>%1</b> [%2]: %3")
                             .arg(author, timestamp, message.toHtmlEscaped());
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

  if (!grpcClient_) {
    QMessageBox::warning(this, "Client not ready",
                         "The gRPC client is not ready yet. Try reconnecting.");
    return;
  }

  const auto status = grpcClient_->sendMessage(text.toStdString());
  if (!status.ok()) {
    QMessageBox::warning(this, "Send failed",
                         QString::fromStdString(status.error_message()));
  }
}

void ChatWindow::handleConnect() {
  const auto pseudonym = pseudonymInput_->text().trimmed();
  const auto gender = genderInput_->text().trimmed();
  const auto country = countryInput_->text().trimmed();

  if (pseudonym.isEmpty() || gender.isEmpty() || country.isEmpty()) {
    QMessageBox::warning(
        this, "Incomplete details",
        "Please provide pseudonym, gender, and country before connecting.");
    return;
  }

  if (!grpcClient_) {
    grpcClient_ = std::make_unique<GrpcChatClient>(kServerAddress);
  }

  connectButton_->setEnabled(false);

  const auto result = grpcClient_->connect(
      pseudonym.toStdString(), gender.toStdString(), country.toStdString());

  connectButton_->setEnabled(true);

  if (!result.status.ok()) {
    QMessageBox::critical(
        this, "Connection failed",
        QStringLiteral("Unable to connect to %1.\n%2")
            .arg(QString::fromUtf8(kServerAddress))
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
}

void ChatWindow::switchToChatView(const QString &welcomeMessage) {
  stacked_->setCurrentWidget(chatView_);
  conversation_->clear();

  addMessage("System", QStringLiteral("Connected as %1 from %2")
                           .arg(pseudonymInput_->text().trimmed(),
                                countryInput_->text().trimmed()));

  if (!welcomeMessage.trimmed().isEmpty()) {
    addMessage("Server", welcomeMessage.trimmed());
  }

  input_->setFocus();
}

void ChatWindow::startMessageStream() {
  if (!grpcClient_) {
    return;
  }

  stopMessageStream();

  grpcClient_->startMessageStream(
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
  if (grpcClient_) {
    grpcClient_->stopMessageStream();
  }
}
