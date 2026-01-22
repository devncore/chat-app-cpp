#include "ui/private_chat_window.hpp"

#include <QDateTime>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QTextBrowser>
#include <QVBoxLayout>

PrivateChatWindow::PrivateChatWindow(const QString &localUser,
                                     const QString &remoteUser, QWidget *parent)
    : QWidget(parent, Qt::Window), localUser_(localUser),
      remoteUser_(remoteUser) {
  setWindowTitle(QStringLiteral("Private chat with %1").arg(remoteUser_));
  resize(400, 300);

  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(12, 12, 12, 12);
  layout->setSpacing(8);

  conversation_ = new QTextBrowser(this);
  conversation_->setReadOnly(true);
  conversation_->setPlaceholderText(
      QStringLiteral("Private conversation with %1...").arg(remoteUser_));
  conversation_->setOpenExternalLinks(true);
  layout->addWidget(conversation_, 1);

  auto *inputLayout = new QHBoxLayout();
  inputLayout->setContentsMargins(0, 0, 0, 0);

  input_ = new QLineEdit(this);
  input_->setPlaceholderText("Type a private message...");
  inputLayout->addWidget(input_);

  sendButton_ = new QPushButton("Send", this);
  sendButton_->setDefault(true);
  inputLayout->addWidget(sendButton_);

  layout->addLayout(inputLayout);

  connect(sendButton_, &QPushButton::clicked, this,
          &PrivateChatWindow::handleSend);
  connect(input_, &QLineEdit::returnPressed, this,
          &PrivateChatWindow::handleSend);
}

QString PrivateChatWindow::remoteUser() const { return remoteUser_; }

void PrivateChatWindow::addMessage(const QString &author,
                                   const QString &message) {
  const auto timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
  const auto color = (author == localUser_) ? "darkgreen" : "darkblue";
  const auto formatted =
      QStringLiteral("<span style='color: %1;'><b>%2</b> [%3]: %4</span>")
          .arg(color, author, timestamp, message.toHtmlEscaped());
  conversation_->append(formatted);
}

void PrivateChatWindow::handleSend() {
  const auto text = input_->text().trimmed();
  if (text.isEmpty()) {
    return;
  }

  input_->clear();
  addMessage(localUser_, text);
  emit sendPrivateMessageRequested(remoteUser_, text);
}
