#include <QApplication>
#include <QCommandLineParser>
#include <QFile>
#include <QTextStream>

#include "service/chat_service_grpc.hpp"
#include "ui/chat_window.hpp"
#include "ui/login_view.hpp"
#include "ui/stacked_widget_handler.hpp"

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
  ChatWindow window;
  auto *loginView = new LoginView(serverAddress, &window);
  auto *handler =
      new StackedWidgetHandler(loginView, window.chatView(), &window);
  window.setHandler(handler);
  auto *grpcChatClient =
      new ChatServiceGrpc(serverAddress.toStdString(), &window);

  // signals LoginView -> grpc
  QObject::connect(loginView, &LoginView::connectRequested, grpcChatClient,
                   &ChatServiceGrpc::connectToServer);

  // signals grpc -> LoginView
  QObject::connect(grpcChatClient, &ChatServiceGrpc::connectFinished, loginView,
                   &LoginView::onConnectFinished);

  // signals LoginView -> ChatWindow
  QObject::connect(loginView, &LoginView::loginSucceeded, &window,
                   &ChatWindow::onLoginSucceeded);

  // signals ChatWindow -> grpc
  QObject::connect(&window, &ChatWindow::disconnectRequested, grpcChatClient,
                   &ChatServiceGrpc::disconnectFromServer);
  QObject::connect(&window, &ChatWindow::sendMessageRequested, grpcChatClient,
                   &ChatServiceGrpc::sendChatMessage);
  QObject::connect(&window, &ChatWindow::startMessageStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::startMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::stopMessageStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::stopMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::startClientEventStreamRequested,
                   grpcChatClient,
                   &ChatServiceGrpc::startClientEventStreamSlot);
  QObject::connect(&window, &ChatWindow::stopClientEventStreamRequested,
                   grpcChatClient, &ChatServiceGrpc::stopClientEventStreamSlot);

  // signals grpc -> ChatWindow
  QObject::connect(grpcChatClient, &ChatServiceGrpc::disconnectFinished,
                   &window, &ChatWindow::onDisconnectFinished);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::sendMessageFinished,
                   &window, &ChatWindow::onSendMessageFinished);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::messageReceived, &window,
                   &ChatWindow::onMessageReceived);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::messageStreamError,
                   &window, &ChatWindow::onMessageStreamError);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::clientEventReceived,
                   &window, &ChatWindow::onClientEventReceived);
  QObject::connect(grpcChatClient, &ChatServiceGrpc::clientEventStreamError,
                   &window, &ChatWindow::onClientEventStreamError);

  window.show();

  return app.exec();
}
