#pragma once

#include <QMainWindow>
#include <QString>

#include <memory>

namespace database {
class IDatabaseManager;
}

class QAction;
class QCloseEvent;
class QDockWidget;
class QToolBar;
class LoginView;
class ChatWindow;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(const QString &serverAddress,
             std::shared_ptr<database::IDatabaseManager> dbManager,
             QWidget *parent = nullptr);
  ~MainWindow() override;

  LoginView *loginView() const;
  ChatWindow *chatWindow() const;
  database::IDatabaseManager *databaseManager() const;

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
  std::shared_ptr<database::IDatabaseManager> dbManager_;
};
