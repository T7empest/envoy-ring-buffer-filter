#include "echo2.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

Network::FilterStatus Echo2Filter::onData(Buffer::Instance& data, bool) {
  // Echo back whatever was received
  read_callbacks_->connection().write(data, false);
  return Network::FilterStatus::Continue;
}

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

