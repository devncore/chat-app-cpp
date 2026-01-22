#pragma once

#include <QString>
#include <QWidget>

class QLineEdit;
class QPushButton;
class QTextBrowser;

class PrivateChatWindow : public QWidget {
  Q_OBJECT

public:
  explicit PrivateChatWindow(const QString &localUser,
                             const QString &remoteUser,
                             QWidget *parent = nullptr);
  ~PrivateChatWindow() override = default;

  QString remoteUser() const;

  void addMessage(const QString &author, const QString &message);

signals:
  void sendPrivateMessageRequested(const QString &recipient,
                                   const QString &content);

private:
  void handleSend();

  QString localUser_;
  QString remoteUser_;
  QTextBrowser *conversation_;
  QLineEdit *input_;
  QPushButton *sendButton_;
};
