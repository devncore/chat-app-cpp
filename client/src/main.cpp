#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>

#include "service/chat_service_grpc.hpp"
#include "ui/chat_window.hpp"
#include "ui/login_view.hpp"
#include "ui/main_window.hpp"

namespace {
QString getServerAddressFromArguments(const QApplication &app) {
  QCommandLineParser parser;
  parser.setApplicationDescription("Chat gRPC client");
  parser.addHelpOption();
  QCommandLineOption serverOption({"s", "server"},
                                  "gRPC server address (host:port).", "address",
                                  "localhost:50051");
  parser.addOption(serverOption);
  parser.process(app);

  return parser.value(serverOption);
}

void useExternalStyleSheet(QApplication &app, const QString &cssFilePath) {
  QFile file(cssFilePath);
  if (file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream stream(&file);
    app.setStyleSheet(stream.readAll());
  } else {
    qDebug() << "Fails to open style sheet?=.\n";
  }
}
} // namespace

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // parse command line arguments
  const QString serverAddress = getServerAddressFromArguments(app);

  // apply external stylesheet
  useExternalStyleSheet(app, "./client/src/ui/style.css");

  // instantiate components
  MainWindow mainWindow(serverAddress);
  auto *loginView = mainWindow.loginView();
  auto *chatWindow = mainWindow.chatWindow();
  auto *grpcChatClient =
      new ChatServiceGrpc(serverAddress.toStdString(), &mainWindow);

  // signals LoginView -> grpc
  QObject::connect(loginView, &LoginView::connectRequested, grpcChatClient,
                   &ChatServiceGrpc::connectToServer);

  // signals grpc -> LoginView
  QObject::connect(grpcChatClient, &ChatServiceGrpc::connectFinished, loginView,
                   &LoginView::onConnectFinished);

  // signals LoginView -> ChatWindow
  QObject::connect(loginView, &LoginView::loginSucceeded, chatWindow,
                   &ChatWindow::onLoginSucceeded);

  // signals ChatWindow -> grpc
  QObject::connect(chatWindow, &ChatWindow::disconnectRequested, grpcChatClient,
                   &ChatServiceGrpc::disconnectFromServer);
  QObject::connect(chatWindow, &ChatWindow::sendMessageRequested, grpcChatClient,
                   &ChatServiceGrpc::sendChatMessage);
  QObject::connect(chatWindow, &ChatWindow::startMessageStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::startMessageStreamSlot);
  QObject::connect(chatWindow, &ChatWindow::stopMessageStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::stopMessageStreamSlot);
  QObject::connect(chatWindow, &ChatWindow::startClientEventStreamRequested,
                   grpcChatClient,
                   &ChatServiceGrpc::startClientEventStreamSlot);
  QObject::connect(chatWindow, &ChatWindow::stopClientEventStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::stopClientEventStreamSlot);

  // signals grpc -> ChatWindow
  QObject::connect(grpcChatClient, &ChatServiceGrpc::disconnectFinished,
                   chatWindow, &ChatWindow::onDisconnectFinished);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::sendMessageFinished,
                   chatWindow, &ChatWindow::onSendMessageFinished);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::messageReceived, chatWindow,
                   &ChatWindow::onMessageReceived);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::messageStreamError,
                   chatWindow, &ChatWindow::onMessageStreamError);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::clientEventReceived,
                   chatWindow, &ChatWindow::onClientEventReceived);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::clientEventStreamError,
                   chatWindow, &ChatWindow::onClientEventStreamError);

  mainWindow.show();

  return app.exec();
}
