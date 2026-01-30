#include "ui/main_window.hpp"

#include <QAction>
#include <QCloseEvent>
#include <QDockWidget>
#include <QMessageBox>
#include <QToolBar>

#include "database/database_manager.hpp"
#include "ui/chat_window.hpp"
#include "ui/login_view.hpp"

MainWindow::MainWindow(const QString &serverAddress,
                       std::shared_ptr<database::IDatabaseManager> dbManager,
                       QWidget *parent)
    : QMainWindow(parent), dbManager_(std::move(dbManager)) {
  setWindowTitle("Chat Client");
  resize(640, 480);

  chatWindow_ = new ChatWindow(dbManager_, this);
  setCentralWidget(chatWindow_);
  chatWindow_->hide();

  loginView_ = new LoginView(serverAddress, this);

  loginDock_ = new QDockWidget("Login", this);
  loginDock_->setWidget(loginView_);
  loginDock_->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
  loginDock_->setFeatures(QDockWidget::DockWidgetMovable);
  addDockWidget(Qt::LeftDockWidgetArea, loginDock_);

  connect(chatWindow_, &ChatWindow::loginCompleted, this,
          &MainWindow::onLoginCompleted);

  chatToolBar_ = addToolBar("Chat");
  chatToolBar_->setMovable(false);
  disconnectAction_ = chatToolBar_->addAction("Disconnect");
  disconnectAction_->setIcon(QIcon("./client/src/ui/icons/disconnect.svg"));
  connect(disconnectAction_, &QAction::triggered, this,
          &MainWindow::onDisconnectTriggered);
  chatToolBar_->hide();
}

MainWindow::~MainWindow() = default;

LoginView *MainWindow::loginView() const { return loginView_; }

ChatWindow *MainWindow::chatWindow() const { return chatWindow_; }

database::IDatabaseManager *MainWindow::databaseManager() const {
  return dbManager_.get();
}

void MainWindow::onLoginCompleted() {

  // database intialization error handling
  if (not dbManager_->isInitialized()) {
    if (auto error = dbManager_->init(loginView_->pseudonym().toStdString())) {
      qDebug() << "Database initialization failed:" << error->c_str();
      QMessageBox::critical(this, "Database Error",
                            "Cannot access the database. The application will "
                            "close.\n\nError: " +
                                QString::fromStdString(*error));
      close();
      return;
    }
  }

  // manage widgets visibility
  loginDock_->hide();
  chatWindow_->show();
  chatToolBar_->show();
  setWindowTitle(
      QStringLiteral("Chat Client - %1").arg(loginView_->pseudonym()));
}

void MainWindow::onDisconnectTriggered() {
  chatWindow_->prepareClose();
  chatWindow_->hide();
  chatToolBar_->hide();
  loginDock_->show();
  dbManager_->resetConnection();
  setWindowTitle("Chat Client");
}

void MainWindow::closeEvent(QCloseEvent *event) {
  chatWindow_->prepareClose();
  QMainWindow::closeEvent(event);
}
