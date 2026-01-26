#include "ui/login_view.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

#include <utility>

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
  layout->addWidget(connectButton_);

  connect(connectButton_, &QPushButton::clicked, this,
          [this]() { handleConnect(); });
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
