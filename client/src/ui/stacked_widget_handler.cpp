#include "ui/stacked_widget_handler.hpp"

#include <QStackedWidget>

StackedWidgetHandler::StackedWidgetHandler(QWidget *loginView,
                                           QWidget *chatView,
                                           QWidget *parent)
    : QObject(parent), stacked_(new QStackedWidget(qobject_cast<QWidget *>(parent))),
      loginView_(loginView), chatView_(chatView) {
  stacked_->addWidget(loginView_);
  stacked_->addWidget(chatView_);
  stacked_->setCurrentWidget(loginView_);
}

QWidget *StackedWidgetHandler::widget() const { return stacked_; }

void StackedWidgetHandler::showLoginView() {
  stacked_->setCurrentWidget(loginView_);
}

void StackedWidgetHandler::showChatView() {
  stacked_->setCurrentWidget(chatView_);
}
