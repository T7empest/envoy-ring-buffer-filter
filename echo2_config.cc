#include "echo2.h"

#include "envoy/server/filter_config.h"
#include "source/common/protobuf/protobuf.h"

namespace Envoy {
namespace Extensions {
namespace NetworkFilters {
namespace Echo2 {

class Echo2ConfigFactory : public Server::Configuration::NamedNetworkFilterConfigFactory {
public:
  absl::StatusOr<Network::FilterFactoryCb>
  createFilterFactoryFromProto(const Protobuf::Message&,
                               Server::Configuration::FactoryContext&) override {
    return [](Network::FilterManager& filter_manager) {
      filter_manager.addReadFilter(std::make_shared<Echo2Filter>());
    };
  }

  ProtobufTypes::MessagePtr createEmptyConfigProto() override {
    return std::make_unique<ProtobufWkt::Struct>();
  }

  std::string name() const override { return "envoy.filters.network.echo2"; }
};

REGISTER_FACTORY(Echo2ConfigFactory,
                 Server::Configuration::NamedNetworkFilterConfigFactory);

} // namespace Echo2
} // namespace NetworkFilters
} // namespace Extensions
} // namespace Envoy

