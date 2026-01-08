#pragma once

#include <QWidget>

#include <memory>
#include <string>

class QFormLayout;
class QLineEdit;
class QComboBox;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTextBrowser;
class QCloseEvent;

class GrpcChatClient;

namespace chat {
class ClientEventData;
}

class ChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit ChatWindow(QWidget *parent = nullptr);
  ~ChatWindow() override;

protected:
  void closeEvent(QCloseEvent *event) override;

private:
  static inline QString MESSAGE_COLOR_SYSTEM{"blue"};
  static inline QString MESSAGE_COLOR_USER_CONNECT{"green"};
  static inline QString MESSAGE_COLOR_USER_DISCONNECT{"purple"};

  QWidget *createLoginView();
  QWidget *createChatView();
  void addMessage(const QString &author, const QString &message,
                  QString color = "black");
  void handleSend();
  void handleConnect();
  void switchToChatView(const QString &welcomeMessage);
  bool addClientToList(const QString &pseudonym);
  bool removeClientFromList(const QString &pseudonym);
  void handleClientEvent(const chat::ClientEventData &eventData);
  void startMessageStream();
  void stopMessageStream();
  void startClientEventStream();
  void stopClientEventStream();

  QStackedWidget *stacked_;
  QWidget *loginView_;
  QWidget *chatView_;
  QLineEdit *pseudonymInput_;
  QComboBox *genderInput_;
  QLineEdit *countryInput_;
  QPushButton *connectButton_;
  QTextBrowser *conversation_;
  QListWidget *clientsList_{nullptr};
  QLineEdit *input_;
  QPushButton *sendButton_;
  bool connected_{false};

  std::unique_ptr<GrpcChatClient> grpcClient_;
};
