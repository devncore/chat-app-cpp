#include <QApplication>
#include <QCommandLineParser>

#include "service/chat_service_grpc.hpp"
#include "ui/chat_window.hpp"

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
} // namespace

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // parse command line arguments
  const QString serverAddress = getServerAddressFromArguments(app);

  // instanciate grpc & chat client components
  ChatWindow window(serverAddress);
  ChatServiceGrpc *grpcChatClient =
      new ChatServiceGrpc(serverAddress.toStdString(), &window);

  // signals UI -> grpcgrpcChatClient
  QObject::connect(&window, &ChatWindow::connectRequested, grpcChatClient,
                   &ChatServiceGrpc::connectToServer);
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
  // signals grpc -> UI
  QObject::connect(grpcChatClient, &ChatServiceGrpc::connectFinished, &window,
                   &ChatWindow::onConnectFinished);
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
