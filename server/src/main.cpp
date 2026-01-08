#include <grpcpp/grpcpp.h>

#include <QCoreApplication>
#include <QThread>
#include <iostream>
#include <memory>
#include <string>
#include <thread>

#include "database_manager.hpp"
#include "service/chat_service.hpp"

int main(int argc, char **argv) {

  QCoreApplication app(argc, argv);

  // instanciate db thread
  auto dbThread = std::make_unique<QThread>();
  auto dbMgr = std::make_shared<database::DatabaseManager>();
  dbMgr->moveToThread(dbThread.get());
  dbThread->start();

  // initialize DB inside the thread
  QMetaObject::invokeMethod(dbMgr.get(), "init", Qt::QueuedConnection);
  QMetaObject::invokeMethod(dbMgr.get(), "printStatisticsTableContent",
                            Qt::QueuedConnection);

  // grpc thread configuration, instanciation and start
  std::thread grpcThread([dbMgrLambda = dbMgr]() {
    const std::string server_address{"0.0.0.0:50051"};
    ChatServiceImpl service(dbMgrLambda);
    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (!server) {
      std::cerr << "Failed to start gRPC server." << std::endl;
      return 1;
    }

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();

    return 0;
  });

  // run Qt event loop (handles queued calls to DatabaseManager)
  int ret = app.exec();

  // clean-up
  grpcThread.join();
  dbThread->quit();
  dbThread->wait();

  return ret;
}
