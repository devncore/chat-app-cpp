#include <QApplication>
#include <QCommandLineParser>

#include "chat_window.hpp"
#include "grpc_chat_client.hpp"

static QString getServerAddressFromArguments(const QApplication &app) {
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

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // parse command line arguments
  const QString serverAddress = getServerAddressFromArguments(app);

  // instanciate grpc & chat client components
  ChatWindow window(serverAddress);
  GrpcChatClient *grpcChatClient =
      new GrpcChatClient(serverAddress.toStdString(), &window);

  // signals UI -> grpcgrpcChatClient
  QObject::connect(&window, &ChatWindow::connectRequested, grpcChatClient,
                   &GrpcChatClient::connectToServer);
  QObject::connect(&window, &ChatWindow::disconnectRequested, grpcChatClient,
                   &GrpcChatClient::disconnectFromServer);
  QObject::connect(&window, &ChatWindow::sendMessageRequested, grpcChatClient,
                   &GrpcChatClient::sendChatMessage);
  QObject::connect(&window, &ChatWindow::startMessageStreamRequested,
                   grpcChatClient, &GrpcChatClient::startMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::stopMessageStreamRequested,
                   grpcChatClient, &GrpcChatClient::stopMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::startClientEventStreamRequested,
                   grpcChatClient, &GrpcChatClient::startClientEventStreamSlot);
  QObject::connect(&window, &ChatWindow::stopClientEventStreamRequested,
                   grpcChatClient, &GrpcChatClient::stopClientEventStreamSlot);
  // signals grpc -> UI
  QObject::connect(grpcChatClient, &GrpcChatClient::connectFinished, &window,
                   &ChatWindow::onConnectFinished);
  QObject::connect(grpcChatClient, &GrpcChatClient::disconnectFinished, &window,
                   &ChatWindow::onDisconnectFinished);
  QObject::connect(grpcChatClient, &GrpcChatClient::sendMessageFinished,
                   &window, &ChatWindow::onSendMessageFinished);
  QObject::connect(grpcChatClient, &GrpcChatClient::messageReceived, &window,
                   &ChatWindow::onMessageReceived);
  QObject::connect(grpcChatClient, &GrpcChatClient::messageStreamError, &window,
                   &ChatWindow::onMessageStreamError);
  QObject::connect(grpcChatClient, &GrpcChatClient::clientEventReceived,
                   &window, &ChatWindow::onClientEventReceived);
  QObject::connect(grpcChatClient, &GrpcChatClient::clientEventStreamError,
                   &window, &ChatWindow::onClientEventStreamError);

  window.show();

  return app.exec();
}
