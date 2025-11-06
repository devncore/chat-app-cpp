#pragma once

#include <QWidget>

#include <memory>
#include <string>

class QFormLayout;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTextBrowser;

class GrpcChatClient;

class ChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit ChatWindow(QWidget *parent = nullptr);
  ~ChatWindow() override;

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
  bool connected_{false};

  std::unique_ptr<GrpcChatClient> grpcClient_;
};
