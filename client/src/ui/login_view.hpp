#pragma once

#include <QString>
#include <QStringList>
#include <QWidget>

class QLineEdit;
class QComboBox;
class QPushButton;

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

public slots:
  void onConnectFinished(bool ok, const QString &errorText, bool accepted,
                         const QString &message,
                         const QStringList &connectedPseudonyms);

private:
  void handleConnect();

  QLineEdit *pseudonymInput_;
  QComboBox *genderInput_;
  QLineEdit *countryInput_;
  QPushButton *connectButton_;
  QString serverAddress_;
};
