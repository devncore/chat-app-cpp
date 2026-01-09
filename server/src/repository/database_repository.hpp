#pragma once

#include <string>

namespace database {

/**
 * @brief Interface for persistence operations used by the server.
 */
class IDatabaseRepository {
public:
  /**
   * @brief Virtual destructor for interface cleanup.
   */
  virtual ~IDatabaseRepository() = default;

  /**
   * @brief Record a client connection event for the given pseudonym.
   * @param pseudonymStd The client's pseudonym.
   */
  virtual void clientConnectionEvent(const std::string &pseudonymStd) = 0;

  /**
   * @brief Increment transmitted message count for the given pseudonym.
   * @param pseudonymStd The client's pseudonym.
   */
  virtual void incrementTxMessage(const std::string &pseudonymStd) = 0;

  /**
   * @brief Emit the current statistics table content.
   */
  virtual void printStatisticsTableContent() = 0;
};

} // namespace database
