#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

class QFormLayout;
class QLineEdit;
class QComboBox;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QTextBrowser;
class QCloseEvent;

class ChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit ChatWindow(QString serverAddress, QWidget *parent = nullptr);
  ~ChatWindow() override;

protected:
  void closeEvent(QCloseEvent *event) override;

signals:
  void connectRequested(const QString &pseudonym, const QString &gender,
                        const QString &country);
  void disconnectRequested(const QString &pseudonym);
  void sendMessageRequested(const QString &content);
  void startMessageStreamRequested();
  void stopMessageStreamRequested();
  void startClientEventStreamRequested();
  void stopClientEventStreamRequested();

public slots:
  void onConnectFinished(bool ok, const QString &errorText, bool accepted,
                         const QString &message,
                         const QStringList &connectedPseudonyms);
  void onDisconnectFinished(bool ok, const QString &errorText);
  void onSendMessageFinished(bool ok, const QString &errorText);
  void onMessageReceived(const QString &author, const QString &content);
  void onMessageStreamError(const QString &errorText);
  void onClientEventReceived(int eventType, const QString &pseudonym);
  void onClientEventStreamError(const QString &errorText);

private:
  static inline QString MESSAGE_COLOR_SYSTEM_{"blue"};
  static inline QString MESSAGE_COLOR_USER_CONNECT_{"green"};
  static inline QString MESSAGE_COLOR_USER_DISCONNECT_{"purple"};

  QWidget *createLoginView();
  QWidget *createChatView();
  void addMessage(const QString &author, const QString &message,
                  QString color = "black");
  void handleSend();
  void handleConnect();
  void switchToChatView(const QString &welcomeMessage,
                        const QStringList &connectedPseudonyms);
  bool addClientToList(const QString &pseudonym);
  bool removeClientFromList(const QString &pseudonym);
  void handleClientEvent(int eventType, const QString &pseudonym);
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
  QString serverAddress_;
};
