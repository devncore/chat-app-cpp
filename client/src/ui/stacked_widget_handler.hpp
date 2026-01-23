#pragma once

#include <QObject>

class QStackedWidget;
class QWidget;

class StackedWidgetHandler : public QObject {
  Q_OBJECT

public:
  explicit StackedWidgetHandler(QWidget *loginView, QWidget *chatView,
                                QWidget *parent = nullptr);

  QWidget *widget() const;

public slots:
  void showLoginView();
  void showChatView();

private:
  QStackedWidget *stacked_;
  QWidget *loginView_;
  QWidget *chatView_;
};
