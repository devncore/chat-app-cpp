#pragma once

#include <memory>
#include <optional>

#include <QHash>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QWidget>

namespace database {
class IDatabaseManager;
}

class ClientListHelper;
class QAction;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QMenu;
class QPushButton;
class QTextBrowser;
class PrivateChatWindow;

class ChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit ChatWindow(std::shared_ptr<database::IDatabaseManager> dbManager,
                      QWidget *parent = nullptr);
  ~ChatWindow() override;

public:
  void prepareClose();

signals:
  void loginCompleted();
  void disconnectRequested(const QString &pseudonym);
  void sendMessageRequested(
      const QString &content,
      const std::optional<QString> &privateRecipient = std::nullopt);
  void startMessageStreamRequested();
  void stopMessageStreamRequested();
  void startClientEventStreamRequested();
  void stopClientEventStreamRequested();

public slots:
  void onLoginSucceeded(const QString &pseudonym, const QString &country,
                        const QString &welcomeMessage,
                        const QStringList &connectedPseudonyms);
  void onDisconnectFinished(bool ok, const QString &errorText);
  void onSendMessageFinished(bool ok, const QString &errorText);
  void onMessageReceived(const QString &author, const QString &content,
                         bool isPrivate);
  void onMessageStreamError(const QString &errorText);
  void onClientEventReceived(int eventType, const QString &pseudonym);
  void onClientEventStreamError(const QString &errorText);
  void onUserUnbanned(const QString &pseudonym);

private:
  static inline QString MESSAGE_COLOR_SYSTEM_{"blue"};
  static inline QString MESSAGE_COLOR_USER_CONNECT_{"green"};
  static inline QString MESSAGE_COLOR_USER_DISCONNECT_{"purple"};

  void setupUi();
  void addMessage(const QString &author, const QString &message,
                  QString color = "black");
  void addPrivateMessage(const QString &author, const QString &message);
  void handleSend();
  void initChatView(const QString &welcomeMessage,
                    const QStringList &connectedPseudonyms);
  void handleClientEvent(int eventType, const QString &pseudonym);
  void startMessageStream();
  void stopMessageStream();
  void startClientEventStream();
  void stopClientEventStream();
  void showClientsContextMenu(const QPoint &pos);
  void openPrivateChatWith(const QString &pseudonym);
  void banUnbanUser(QListWidgetItem *item);
  void onPrivateMessageRequested(const QString &recipient,
                                 const QString &content);

  QTextBrowser *conversation_;
  QListWidget *clientsList_{nullptr};
  QLineEdit *input_;
  QPushButton *sendButton_;
  bool connected_{false};
  QString pseudonym_;
  QString country_;
  QMenu *clientsContextMenu_{nullptr};
  QAction *banUnbanAction_{nullptr};
  std::shared_ptr<database::IDatabaseManager> dbManager_;
  std::unique_ptr<ClientListHelper> clientListHelper_;
  QHash<QString, QPointer<PrivateChatWindow>> privateChats_;
};
