#include "ui/login_view.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QShortcut>
#include <QTimer>
#include <QVBoxLayout>

LoginView::LoginView(QString serverAddress, QWidget *parent)
    : QWidget(parent), serverAddress_(std::move(serverAddress)) {
  auto *layout = new QVBoxLayout(this);
  layout->setContentsMargins(48, 48, 48, 48);

  auto *formLayout = new QFormLayout();
  formLayout->setLabelAlignment(Qt::AlignRight);
  formLayout->setFormAlignment(Qt::AlignTop);

  pseudonymInput_ = new QLineEdit(this);
  pseudonymInput_->setText("John");
  formLayout->addRow("Pseudonym:", pseudonymInput_);

  genderInput_ = new QComboBox(this);
  genderInput_->addItem("Male");
  genderInput_->addItem("Female");
  genderInput_->setCurrentText("Male");
  formLayout->addRow("Gender:", genderInput_);

  countryInput_ = new QLineEdit(this);
  countryInput_->setText("France");
  formLayout->addRow("Country:", countryInput_);

  layout->addLayout(formLayout);

  connectButton_ = new QPushButton("Connect", this);
  connectButton_->setDefault(true);
  connectButton_->setEnabled(false);
  layout->addWidget(connectButton_);

  layout->addStretch();

  serverStatusLabel_ = new QLabel("", this);
  serverStatusLabel_->setStyleSheet("font-size: 10px;");
  layout->addWidget(serverStatusLabel_, 0, Qt::AlignRight | Qt::AlignBottom);

  connect(connectButton_, &QPushButton::clicked, this,
          [this]() { handleConnect(); });

  auto *enterShortcut = new QShortcut(QKeySequence(Qt::Key_Return), this);
  connect(enterShortcut, &QShortcut::activated, this,
          [this]() { handleConnect(); });

  auto *numpadEnterShortcut = new QShortcut(QKeySequence(Qt::Key_Enter), this);
  connect(numpadEnterShortcut, &QShortcut::activated, this,
          [this]() { handleConnect(); });

  serverCheckTimer_ = new QTimer(this);
  serverCheckTimer_->setInterval(1000);
  connect(serverCheckTimer_, &QTimer::timeout, this,
          [this]() { emit checkServerAvailabilityRequested(); });
  serverCheckTimer_->start();
}

QString LoginView::pseudonym() const {
  return pseudonymInput_->text().trimmed();
}

QString LoginView::country() const { return countryInput_->text().trimmed(); }

void LoginView::handleConnect() {
  const auto pseudonym = pseudonymInput_->text().trimmed();
  const auto gender = genderInput_->currentText().trimmed();
  const auto country = countryInput_->text().trimmed();

  if (pseudonym.isEmpty() || gender.isEmpty() || country.isEmpty()) {
    QMessageBox::warning(
        this, "Incomplete details",
        "Please provide pseudonym, gender, and country before connecting.");
    return;
  }

  connectButton_->setEnabled(false);
  emit connectRequested(pseudonym, gender, country);
}

void LoginView::onConnectFinished(bool ok, const QString &errorText,
                                  bool accepted, const QString &message,
                                  const QStringList &connectedPseudonyms) {
  connectButton_->setEnabled(true);

  if (!ok) {
    QMessageBox::critical(this, "Connection failed",
                          QStringLiteral("Unable to connect to %1.\n%2")
                              .arg(serverAddress_)
                              .arg(errorText));
    return;
  }

  if (!accepted) {
    QMessageBox::warning(this, "Connection rejected", message);
    return;
  }

  emit loginSucceeded(pseudonymInput_->text().trimmed(),
                      countryInput_->text().trimmed(), message,
                      connectedPseudonyms);
}

void LoginView::onConnectivityStateChanged(int stateValue) {
  const auto maybeState =
      ServerConnectionState::_from_integral_nothrow(stateValue);

  if (!maybeState) {
    qDebug() << "invalid server connectivity state";
    return;
  }

  const ServerConnectionState state = *maybeState;
  const QString &color = serverStateTextColorMap_.at(state);

  serverStatusLabel_->setText(QString(state._to_string()).toLower());
  serverStatusLabel_->setStyleSheet(
      QStringLiteral("font-size: 10px; color: %1;").arg(color));

  // Enable connect button only when channel is 'Ready'
  connectButton_->setEnabled(state == +ServerConnectionState::READY);
}
