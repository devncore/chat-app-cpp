#pragma once

#include "enums/server_connection_state.hpp"

#include <QString>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QComboBox;
class QPushButton;
class QTimer;
class QLabel;

class LoginView : public QWidget {
  Q_OBJECT

public:
  explicit LoginView(QString serverAddress, QWidget *parent = nullptr);
  ~LoginView() override = default;

  QString pseudonym() const;
  QString country() const;

signals:
  void connectRequested(const QString &pseudonym, const QString &gender,
                        const QString &country);
  void loginSucceeded(const QString &pseudonym, const QString &country,
                      const QString &welcomeMessage,
                      const QStringList &connectedPseudonyms);
  void checkServerAvailabilityRequested();

public slots:
  void onConnectFinished(bool ok, const QString &errorText, bool accepted,
                         const QString &message,
                         const QStringList &connectedPseudonyms);
  void onConnectivityStateChanged(int state);

private:
  void handleConnect();

  using Color = QString;
  const std::map<ServerConnectionState, Color> serverStateTextColorMap_ = {
      {+ServerConnectionState::IDLE, "red"},
      {+ServerConnectionState::CONNECTING, "orange"},
      {+ServerConnectionState::READY, "green"},
      {+ServerConnectionState::TRANSIENT_FAILURE, "red"},
      {+ServerConnectionState::SHUTDOWN, "red"},
      {+ServerConnectionState::UNKNOWN, "red"},
  };

  QLineEdit *pseudonymInput_;
  QComboBox *genderInput_;
  QLineEdit *countryInput_;
  QPushButton *connectButton_;
  QLabel *serverStatusLabel_;
  QTimer *serverCheckTimer_;
  QString serverAddress_;
};
