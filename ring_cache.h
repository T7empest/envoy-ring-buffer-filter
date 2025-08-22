#pragma once

#include "envoy/http/filter.h"
#include "source/common/common/logger.h"

namespace Envoy {
  namespace Extensions {
    namespace HttpFilters {
      namespace RingCache {

        // Minimal pass-through HTTP filter skeleton.
        // TODO: add your cache/coalescing state here.
        class RingCacheFilter : public Http::StreamFilter,
                                public Logger::Loggable<Logger::Id::filter> {
        public:
          RingCacheFilter() = default;

          // ---- Decoder (request → upstream) ----
          Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) override;
          Http::FilterDataStatus    decodeData(Buffer::Instance& data, bool end_stream) override;
          Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override;
          void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override;

          // ---- Encoder (upstream → client) ----
          Http::FilterHeadersStatus encodeHeaders(Http::ResponseHeaderMap& headers, bool end_stream) override;
          Http::FilterDataStatus    encodeData(Buffer::Instance& data, bool end_stream) override;
          Http::FilterTrailersStatus encodeTrailers(Http::ResponseTrailerMap& trailers) override;
          void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override;

          // --- extra encoder hooks required by current Envoy API ---
          Http::Filter1xxHeadersStatus encode1xxHeaders(Http::ResponseHeaderMap& headers) override;
          Http::FilterMetadataStatus   encodeMetadata(Http::MetadataMap& metadata_map) override;

          void onDestroy() override;

        private:
          // TODO: add fields like cache key, entry handle, etc.
          Http::StreamDecoderFilterCallbacks* dec_cb_{nullptr};
          Http::StreamEncoderFilterCallbacks* enc_cb_{nullptr};
        };

      } // namespace RingCache
    } // namespace HttpFilters
  } // namespace Extensions
} // namespace Envoy
