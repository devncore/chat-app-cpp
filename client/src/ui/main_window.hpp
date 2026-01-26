#pragma once

#include <QMainWindow>
#include <QString>

class QCloseEvent;
class QDockWidget;
class LoginView;
class ChatWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(const QString &serverAddress, QWidget *parent = nullptr);
  ~MainWindow() override = default;

  LoginView *loginView() const;
  ChatWindow *chatWindow() const;

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void onLoginCompleted();

private:
  QDockWidget *loginDock_;
  LoginView *loginView_;
  ChatWindow *chatWindow_;
};
