#include <grpcpp/grpcpp.h>

#include <QCoreApplication>
#include <QThread>
#include <boost/program_options.hpp>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <thread>

#include "database_manager.hpp"
#include "service/chat_service_impl.hpp"

class ArgumentParser {
public:
  ArgumentParser(int argc, char **argv) {
    namespace po = boost::program_options;

    boost::program_options::options_description desc("Chat gRPC server");
    desc.add_options()("help,h", "show help message")(
        "listen,l",
        po::value<std::string>(&serverAddress_)
            ->default_value(defaultListenServerEndpoint_),
        "gRPC listen address (host:port).");

    try {
      po::variables_map vm;
      // try to parse arguments
      po::store(po::parse_command_line(argc, argv, desc), vm);
      po::notify(vm);
      // help case
      if (vm.count("help") != 0) {
        std::cout << desc << std::endl;
      }
    } catch (const std::exception &ex) {
      std::cerr << "Error parsing command line arguments: " << ex.what()
                << std::endl;
      throw;
    }
  }

  std::optional<std::string> getServerAddress() const {
    return serverAddress_.empty() ? std::nullopt
                                  : std::make_optional(serverAddress_);
  }

private:
  const std::string defaultListenServerEndpoint_{"0.0.0.0:50051"};
  std::string serverAddress_;
};

int main(int argc, char **argv) {

  try {
    // Arguments parsing
    ArgumentParser argParser(argc, argv);
    const auto serverAddress = argParser.getServerAddress();
    if (not serverAddress.has_value()) {
      std::cerr << "Failed to parse server address argument." << std::endl;
      return 1;
    }

    // Qt ... TO BE REMOVED LATER
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
    std::thread grpcThread([dbMgrLambda = dbMgr, serverAddress]() {
      ChatServiceImpl service(dbMgrLambda);
      grpc::ServerBuilder builder;
      builder.AddListeningPort(serverAddress.value(),
                               grpc::InsecureServerCredentials());
      builder.RegisterService(&service);

      std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
      if (!server) {
        std::cerr << "Failed to start gRPC server." << std::endl;
        return 1;
      }

      std::cout << "Server listening on " << serverAddress.value() << std::endl;
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
  } catch (const std::exception &ex) {
    std::cerr << "Exception in main: " << ex.what() << std::endl;
    return 1;
  }
}
