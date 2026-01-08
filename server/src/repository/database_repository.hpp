#pragma once

#include <string>

namespace database {

class IDatabaseRepository {
public:
  virtual ~IDatabaseRepository() = default;

  virtual void clientConnectionEvent(const std::string &pseudonymStd) = 0;
  virtual void incrementTxMessage(const std::string &pseudonymStd) = 0;
  virtual void printStatisticsTableContent() = 0;
};

} // namespace database
