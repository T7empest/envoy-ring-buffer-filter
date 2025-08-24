#pragma once

#include <utility>

#include "envoy/http/filter.h"
#include "source/common/common/logger.h"

namespace Envoy {
  namespace Extensions {
    namespace HttpFilters {
      namespace RingCache {

        struct CacheKey
        {
          std::string host;
          std::string path;

          bool operator==(const CacheKey& other) const
          {
            return host == other.host && path == other.path;
          }

          // for searching in map, wont be using map later but ring buffer TODO: remove?
          bool operator<(const CacheKey& other) const
          {
            return std::tie(host, path) < std::tie(other.host, other.path);
          }
        };

        struct CachedResponse
        {
          std::string body;
          Http::ResponseHeaderMapPtr headers;
          Http::ResponseTrailerMapPtr trailers; // might be needed later?
        };

        struct RingBufferCache
        {
          // TODO: make actual ring buffer, ?move to new file?
          std::map<CacheKey, CachedResponse> cache;
        };

        // Minimal pass-through HTTP filter skeleton.
        // TODO: cache/coalescing state here.
        // TODO: skip caching for non-GET requests
        class RingCacheFilter : public Http::StreamFilter,
                                public Logger::Loggable<Logger::Id::filter>
        {
        public:
          RingCacheFilter(std::shared_ptr<RingBufferCache> sharedCache)
            : sharedCache_(std::move(sharedCache)) {}

          // Decoder (request → upstream)
          Http::FilterHeadersStatus   decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) override;
          Http::FilterDataStatus      decodeData(Buffer::Instance& data, bool end_stream) override;
          Http::FilterTrailersStatus  decodeTrailers(Http::RequestTrailerMap& trailers) override;
          void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override;

          // Encoder (upstream → client)
          Http::FilterHeadersStatus    encodeHeaders(Http::ResponseHeaderMap& headers, bool end_stream) override;
          Http::FilterDataStatus       encodeData(Buffer::Instance& data, bool end_stream) override;
          Http::FilterTrailersStatus   encodeTrailers(Http::ResponseTrailerMap& trailers) override;
          void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override;

          // extra encoder hooks required by current Envoy API
          Http::Filter1xxHeadersStatus encode1xxHeaders(Http::ResponseHeaderMap& headers) override;
          Http::FilterMetadataStatus   encodeMetadata(Http::MetadataMap& metadata_map) override;

          void onDestroy() override;

        private:
          std::shared_ptr<RingBufferCache> sharedCache_;
          CacheKey cache_key_;
          bool should_cache_ = false;

          Http::StreamDecoderFilterCallbacks* dec_cb_{nullptr};
          Http::StreamEncoderFilterCallbacks* enc_cb_{nullptr};
        };

      } // namespace RingCache
    } // namespace HttpFilters
  } // namespace Extensions
} // namespace Envoy
