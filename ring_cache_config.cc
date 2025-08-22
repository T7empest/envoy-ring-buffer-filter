#include "ring_cache.h"

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"
#include "source/common/protobuf/protobuf.h"

namespace Envoy {
  namespace Extensions {
    namespace HttpFilters {
      namespace RingCache {

        // Factory skeleton for the HTTP filter.
        class RingCacheFilterFactory : public Server::Configuration::NamedHttpFilterConfigFactory {
        public:
          ProtobufTypes::MessagePtr createEmptyConfigProto() override {
            // Keep it schema-less for now; swap to a typed proto later if you want.
            return std::make_unique<ProtobufWkt::Struct>();
          }

          absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(
              const Protobuf::Message&, const std::string&,
              Server::Configuration::FactoryContext&) override {

            return [](Http::FilterChainFactoryCallbacks& callbacks) {
              callbacks.addStreamFilter(std::make_shared<RingCacheFilter>());
            };
          }

          std::string name() const override { return "envoy.filters.http.ring_cache"; }
        };

        REGISTER_FACTORY(RingCacheFilterFactory, Server::Configuration::NamedHttpFilterConfigFactory);

      } // namespace RingCache
    } // namespace HttpFilters
  } // namespace Extensions
} // namespace Envoy
