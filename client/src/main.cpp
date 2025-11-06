#include <QApplication>
#include <QDateTime>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QMetaObject>
#include <QPushButton>
#include <QStackedWidget>
#include <QTextBrowser>
#include <QVBoxLayout>
#include <QWidget>

#include <grpcpp/grpcpp.h>

#include <atomic>
#include <memory>
#include <string>
#include <thread>

#include "chat.grpc.pb.h"
#include <google/protobuf/empty.pb.h>

namespace {
constexpr char kServerAddress[] = "localhost:50051";
}

class ChatWindow : public QWidget {
public:
  explicit ChatWindow(QWidget *parent = nullptr);
  ~ChatWindow();

private:
  QWidget *createLoginView();
  QWidget *createChatView();
  void addMessage(const QString &author, const QString &message);
  void handleSend();
  void handleConnect();
  void switchToChatView(const QString &welcomeMessage);
  void startMessageStream();
  void stopMessageStream();

  QStackedWidget *stacked_;
  QWidget *loginView_;
  QWidget *chatView_;
  QLineEdit *pseudonymInput_;
  QLineEdit *genderInput_;
  QLineEdit *countryInput_;
  QPushButton *connectButton_;
  QTextBrowser *conversation_;
  QLineEdit *input_;
  QPushButton *sendButton_;
  bool connected_ = false;

  std::shared_ptr<grpc::Channel> channel_;
  std::unique_ptr<chat::ChatService::Stub> stub_;
  std::atomic<bool> messageStreamRunning_{false};
  std::thread messageStreamThread_;
  std::shared_ptr<grpc::ClientContext> messageStreamContext_;
};

ChatWindow::ChatWindow(QWidget *parent)
    : QWidget(parent), stacked_(new QStackedWidget(this)) {
  setWindowTitle("Chat Client");
  resize(480, 720);

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

  chat::SendMessageRequest request;
  request.set_content(text.toStdString());

  grpc::ClientContext context;
  google::protobuf::Empty response;
  const auto status =
      stub_->InformServerNewMessage(&context, request, &response);

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

  if (!stub_) {
    channel_ =
        grpc::CreateChannel(kServerAddress, grpc::InsecureChannelCredentials());
    stub_ = chat::ChatService::NewStub(channel_);
  }

  connectButton_->setEnabled(false);

  chat::ConnectRequest request;
  request.set_pseudonym(pseudonym.toStdString());
  request.set_gender(gender.toStdString());
  request.set_country(country.toStdString());

  chat::ConnectResponse response;
  grpc::ClientContext context;

  const auto status = stub_->Connect(&context, request, &response);

  connectButton_->setEnabled(true);

  if (!status.ok()) {
    QMessageBox::critical(
        this, "Connection failed",
        QStringLiteral("Unable to connect to %1.\n%2")
            .arg(QString::fromUtf8(kServerAddress))
            .arg(QString::fromStdString(status.error_message())));
    return;
  }

  if (!response.accepted()) {
    QMessageBox::warning(this, "Connection rejected",
                         QString::fromStdString(response.message()));
    return;
  }

  connected_ = true;
  setWindowTitle(QStringLiteral("Chat Client - %1").arg(pseudonym));
  switchToChatView(QString::fromStdString(response.message()));
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
  if (!stub_)
    return;

  stopMessageStream();

  messageStreamRunning_.store(true);
  auto context = std::make_shared<grpc::ClientContext>();
  messageStreamContext_ = context;

  messageStreamThread_ = std::thread([this, context]() {
    chat::InformClientsNewMessageRequest request;

    auto reader = stub_->InformClientsNewMessage(context.get(), request);
    chat::InformClientsNewMessageResponse incoming;

    while (messageStreamRunning_.load() && reader->Read(&incoming)) {
      const auto author = QString::fromStdString(incoming.author());
      const auto content = QString::fromStdString(incoming.content());

      QMetaObject::invokeMethod(
          this, [this, author, content]() { addMessage(author, content); },
          Qt::QueuedConnection);
    }

    const auto status = reader->Finish();
    const bool stillRunning = messageStreamRunning_.exchange(false);

    if (!status.ok() && stillRunning) {
      const auto errorText = QString::fromStdString(status.error_message());
      QMetaObject::invokeMethod(
          this,
          [this, errorText]() {
            addMessage(
                "System",
                QStringLiteral("Message stream stopped: %1").arg(errorText));
          },
          Qt::QueuedConnection);
    }
  });
}

void ChatWindow::stopMessageStream() {
  const bool wasRunning = messageStreamRunning_.exchange(false);
  if (wasRunning && messageStreamContext_) {
    messageStreamContext_->TryCancel();
  }

  if (messageStreamThread_.joinable()) {
    messageStreamThread_.join();
  }

  messageStreamContext_.reset();
}

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  ChatWindow window;
  window.show();

  return app.exec();
}
