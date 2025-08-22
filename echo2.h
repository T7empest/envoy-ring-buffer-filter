#pragma once

#include "envoy/network/filter.h"
#include "source/common/common/logger.h"
#include "envoy/network/connection.h"


namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

class Echo2Filter : public Network::ReadFilter,
                    public Logger::Loggable<Logger::Id::filter> {
public:
  Echo2Filter() = default;

  // Network::ReadFilter
  Network::FilterStatus onData(Buffer::Instance& data, bool) override;
  Network::FilterStatus onNewConnection() override { return Network::FilterStatus::Continue; }
  void initializeReadFilterCallbacks(Network::ReadFilterCallbacks& callbacks) override {
    read_callbacks_ = &callbacks;
  }

private:
  Network::ReadFilterCallbacks* read_callbacks_{};
};

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

