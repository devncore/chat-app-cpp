#include <boost/program_options.hpp>
#include <iostream>
#include <optional>
#include <string>

#include "database/database_manager.hpp"
#include "database/database_manager_factory.hpp"
#include "grpc/grpc_runner.hpp"

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
    // arguments parsing
    ArgumentParser argParser(argc, argv);
    const auto serverAddress = argParser.getServerAddress();
    if (not serverAddress.has_value()) {
      throw std::runtime_error("Invalid server address argument.");
    }

    // db manager instanciation and print
    const auto databaseManagerOrError =
        database::DatabaseManagerFactory::createDatabaseManagerSQLite();
    if (!databaseManagerOrError.has_value()) {
      throw std::runtime_error("Failed to create DatabaseManagerSQLite: " +
                               databaseManagerOrError.error());
    }
    if (const auto error =
            (*databaseManagerOrError)->printStatisticsTableContent()) {
      throw std::runtime_error("Failed to print statistics table content: " +
                               error.value());
    }

    GrpcRunner grpcServer(*databaseManagerOrError, serverAddress.value());
    grpcServer.wait();
    return 0;

  } catch (const std::exception &ex) {
    std::cerr << "Exception in main: " << ex.what() << std::endl;
    return 1;
  }
}
