#pragma once

#include <QMainWindow>
#include <QString>

class QAction;
class QCloseEvent;
class QDockWidget;
class QToolBar;
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
  void onDisconnectTriggered();

private:
  QDockWidget *loginDock_;
  LoginView *loginView_;
  ChatWindow *chatWindow_;
  QToolBar *chatToolBar_;
  QAction *disconnectAction_;
};
