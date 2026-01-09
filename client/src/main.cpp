#include <QApplication>
#include <QCommandLineParser>

#include "chat_window.hpp"
#include "grpc_chat_client.hpp"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  // parse server endpoint arg
  QCommandLineParser parser;
  parser.setApplicationDescription("Chat gRPC client");
  parser.addHelpOption();
  QCommandLineOption serverOption({"s", "server"},
                                  "gRPC server address (host:port).", "address",
                                  "localhost:50051");
  parser.addOption(serverOption);
  parser.process(app);
  const QString serverAddress = parser.value(serverOption);

  // instanciate grpc & chat client components
  ChatWindow window(serverAddress);
  const auto grpcChatClient =
      std::make_unique<GrpcChatClient>(serverAddress.toStdString());
  const auto* const grpcChatClientPointer = grpcChatClient.get();

  // signals UI -> grpcgrpcChatClientPointer
  QObject::connect(&window, &ChatWindow::connectRequested, grpcChatClientPointer,
                   &GrpcChatClient::connectToServer);
  QObject::connect(&window, &ChatWindow::disconnectRequested, grpcChatClientPointer,
                   &GrpcChatClient::disconnectFromServer);
  QObject::connect(&window, &ChatWindow::sendMessageRequested, grpcChatClientPointer,
                   &GrpcChatClient::sendChatMessage);
  QObject::connect(&window, &ChatWindow::startMessageStreamRequested,
                   grpcChatClientPointer, &GrpcChatClient::startMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::stopMessageStreamRequested,
                   grpcChatClientPointer, &GrpcChatClient::stopMessageStreamSlot);
  QObject::connect(&window, &ChatWindow::startClientEventStreamRequested,
                   grpcChatClientPointer, &GrpcChatClient::startClientEventStreamSlot);
  QObject::connect(&window, &ChatWindow::stopClientEventStreamRequested,
                   grpcChatClientPointer, &GrpcChatClient::stopClientEventStreamSlot);
  // signals grpc -> UI
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::connectFinished, &window,
                   &ChatWindow::onConnectFinished);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::disconnectFinished, &window,
                   &ChatWindow::onDisconnectFinished);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::sendMessageFinished,
                   &window, &ChatWindow::onSendMessageFinished);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::messageReceived, &window,
                   &ChatWindow::onMessageReceived);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::messageStreamError, &window,
                   &ChatWindow::onMessageStreamError);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::clientEventReceived,
                   &window, &ChatWindow::onClientEventReceived);
  QObject::connect(grpcChatClientPointer, &GrpcChatClient::clientEventStreamError,
                   &window, &ChatWindow::onClientEventStreamError);

  window.show();

  return app.exec();
}
