#include <QApplication>
#include <QCommandLineParser>

#include "chat_window.hpp"
#include "grpc_chat_client.hpp"

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);

  QCommandLineParser parser;
  parser.setApplicationDescription("Chat gRPC client");
  parser.addHelpOption();
  QCommandLineOption serverOption(
      {"s", "server"}, "gRPC server address (host:port).", "address",
      "localhost:50051");
  parser.addOption(serverOption);
  parser.process(app);

  const QString serverAddress = parser.value(serverOption);

  auto chatSession =
      std::make_unique<GrpcChatClient>(serverAddress.toStdString());
  ChatWindow window(serverAddress, std::move(chatSession));
  window.show();

  return app.exec();
}
