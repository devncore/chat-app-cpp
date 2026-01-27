#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace database {

using OptionalErrorMessage = std::optional<std::string>;

/**
 * @brief Interface for persistence operations used by the server.
 */
class IDatabaseManager {
public:
  /**
   * @brief Virtual destructor for interface cleanup.
   */
  virtual ~IDatabaseManager() = default;

  /**
   * @brief Record a client connection event for the given pseudonym.
   * @param pseudonymStd The client's pseudonym.
   * @return Empty on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  clientConnectionEvent(std::string_view pseudonymStd) noexcept = 0;

  /**
   * @brief Increment transmitted message count for the given pseudonym.
   * @param pseudonymStd The client's pseudonym.
   * @return Empty on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  incrementTxMessage(std::string_view pseudonymStd) noexcept = 0;

  /**
   * @brief Update the cumulated connection time for the given pseudonym.
   * @param pseudonymStd The client's pseudonym.
   * @param durationInSec The duration in seconds to add.
   * @return Empty on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  updateCumulatedConnectionTime(std::string_view pseudonymStd,
                                uint64_t durationInSec) noexcept = 0;

  /**
   * @brief Emit the current statistics table content.
   * @return Empty on success, or error message on failure.
   */
  [[nodiscard]] virtual OptionalErrorMessage
  printStatisticsTableContent() noexcept = 0;
};

} // namespace database
