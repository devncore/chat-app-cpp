#pragma once

#include "database/database_manager.hpp"

#include <functional>
#include <string>
#include <string_view>

namespace mock {

class MockDatabaseManager : public database::IDatabaseManager {
public:
  // Configurable callbacks for each method
  std::function<database::OptionalErrorMessage(std::string_view)>
      clientConnectionEventFn;
  std::function<database::OptionalErrorMessage(std::string_view)>
      incrementTxMessageFn;
  std::function<database::OptionalErrorMessage(std::string_view, uint64_t)>
      updateCumulatedConnectionTimeFn;
  std::function<database::OptionalErrorMessage()> printStatisticsTableContentFn;

  // Call counters
  mutable int clientConnectionEventCalls = 0;
  mutable int incrementTxMessageCalls = 0;
  mutable int updateCumulatedConnectionTimeCalls = 0;
  mutable int printStatisticsTableContentCalls = 0;

  // Last call arguments
  mutable std::string lastPseudonym;
  mutable uint64_t lastDurationInSec = 0;

  [[nodiscard]] database::OptionalErrorMessage
  clientConnectionEvent(std::string_view pseudonymStd) noexcept override {
    ++clientConnectionEventCalls;
    lastPseudonym = pseudonymStd;
    if (clientConnectionEventFn) {
      return clientConnectionEventFn(pseudonymStd);
    }
    return std::nullopt;
  }

  [[nodiscard]] database::OptionalErrorMessage
  incrementTxMessage(std::string_view pseudonymStd) noexcept override {
    ++incrementTxMessageCalls;
    lastPseudonym = pseudonymStd;
    if (incrementTxMessageFn) {
      return incrementTxMessageFn(pseudonymStd);
    }
    return std::nullopt;
  }

  [[nodiscard]] database::OptionalErrorMessage updateCumulatedConnectionTime(
      std::string_view pseudonymStd,
      uint64_t durationInSec) noexcept override {
    ++updateCumulatedConnectionTimeCalls;
    lastPseudonym = pseudonymStd;
    lastDurationInSec = durationInSec;
    if (updateCumulatedConnectionTimeFn) {
      return updateCumulatedConnectionTimeFn(pseudonymStd, durationInSec);
    }
    return std::nullopt;
  }

  [[nodiscard]] database::OptionalErrorMessage
  printStatisticsTableContent() noexcept override {
    ++printStatisticsTableContentCalls;
    if (printStatisticsTableContentFn) {
      return printStatisticsTableContentFn();
    }
    return std::nullopt;
  }

  void reset() {
    clientConnectionEventCalls = 0;
    incrementTxMessageCalls = 0;
    updateCumulatedConnectionTimeCalls = 0;
    printStatisticsTableContentCalls = 0;
    lastPseudonym.clear();
    lastDurationInSec = 0;
    clientConnectionEventFn = nullptr;
    incrementTxMessageFn = nullptr;
    updateCumulatedConnectionTimeFn = nullptr;
    printStatisticsTableContentFn = nullptr;
  }
};

} // namespace mock
