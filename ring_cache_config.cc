#include "ring_cache.h"
#include "ring_buffer.h"

#include "envoy/registry/registry.h"
#include "envoy/server/filter_config.h"
#include "source/common/protobuf/message_validator_impl.h"
#include "source/common/protobuf/protobuf.h"
#include "source/common/protobuf/utility.h"

namespace Envoy::Extensions::HttpFilters::RingCache
{
	namespace // helper functions
	{
		// parse the yaml config which is loaded as a Protobuf struct object in envoy
		std::vector<Pool> parsePoolsFromStruct(const ProtobufWkt::Struct& s)
		{
			std::vector<Pool> pools;

			const auto& fields = s.fields();
			auto it = fields.find("pools");
			if (it == fields.end() || it->second.kind_case() != ProtobufWkt::Value::kListValue)
				return pools; // no pools configured -- no caching

			for (const auto& v : it->second.list_value().values())
			{
				if (v.kind_case() != ProtobufWkt::Value::kStructValue)
					continue;
				const auto& ps = v.struct_value().fields();

				// name
				std::string name;
				if (auto nit = ps.find("name"); nit != ps.end() && nit->second.kind_case() ==
					ProtobufWkt::Value::kStringValue)
				{
					name = nit->second.string_value();
				}
				else
				{
					name = "unnamed";
				}

				// slots
				size_t slots = 256; // default
				if (auto sit = ps.find("slots"); sit != ps.end() && sit->second.kind_case() ==
					ProtobufWkt::Value::kNumberValue)
				{
					double d = sit->second.number_value();
					if (d > 0)
						slots = static_cast<size_t>(d);
				}

				// match.path_prefixes
				std::vector<std::string> prefixes;
				if (auto mit = ps.find("match"); mit != ps.end() && mit->second.kind_case() ==
					ProtobufWkt::Value::kStructValue)
				{
					const auto& ms = mit->second.struct_value().fields();
					if (auto pit = ms.find("path_prefixes"); pit != ms.end() && pit->second.kind_case() ==
						ProtobufWkt::Value::kListValue)
					{
						for (const auto& sv : pit->second.list_value().values())
						{
							if (sv.kind_case() == ProtobufWkt::Value::kStringValue)
								prefixes.push_back((sv.string_value()));
						}
					}
				}

				pools.emplace_back(std::move(name), std::move(prefixes), slots);
			}

			return pools;
		}
	} // namespace (helper functions)

	class RingCacheFilterFactory : public Server::Configuration::NamedHttpFilterConfigFactory,
	                               public Logger::Loggable<Logger::Id::filter>
	{
	public:
		absl::StatusOr<Http::FilterFactoryCb> createFilterFactoryFromProto(
			const Protobuf::Message& message, const std::string&,
			Server::Configuration::FactoryContext&) override
		{
			ProtobufWkt::Struct struct_msg;
			struct_msg.MergeFrom(message);

			auto pools = std::make_shared<std::vector<Pool>>(parsePoolsFromStruct(struct_msg));

			return [pools](Http::FilterChainFactoryCallbacks& callbacks)
			{
				callbacks.addStreamFilter(std::make_shared<RingCacheFilter>(pools));
			};
		}

		ProtobufTypes::MessagePtr createEmptyConfigProto() override
		{
			return std::make_unique<ProtobufWkt::Struct>();
		}

		std::string name() const override { return "envoy.filters.http.ring_cache"; }
	};

	REGISTER_FACTORY(RingCacheFilterFactory, Server::Configuration::NamedHttpFilterConfigFactory);
} // namespace Envoy::Extensions::HttpFilters::RingCache
