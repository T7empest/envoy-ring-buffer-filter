#include "ring_cache.h"

namespace Envoy {
  namespace Extensions {
    namespace HttpFilters {
      namespace RingCache {

        // Decoder
        Http::FilterHeadersStatus RingCacheFilter::decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream)
        {
#ifndef NDEBUG
          ENVOY_LOG(debug,"decodeHeaders on request: [method={}, path={}, end_stream={}]",
                    headers.getMethodValue(), headers.getPathValue(), end_stream);
#endif

          // reject caching for streamed requests
          if (!end_stream)
            return Http::FilterHeadersStatus::Continue;

          // reject caching of non-GET requests
          if (headers.getMethodValue() != "GET")
            return Http::FilterHeadersStatus::Continue;

          cache_key_ = {
            .host = std::string(headers.getHostValue()),
            .path = std::string(headers.getPathValue())
          };

          // if cache hit -> shortcircuit and server client directly from cache
          auto it = sharedCache_->cache.find(cache_key_);
          if (it != sharedCache_->cache.end())
          {
            const auto& entry = it->second;
            std::function<void(Http::ResponseHeaderMap&)> add_hitStatus_ = [](Http::ResponseHeaderMap& headers)
              { headers.addCopy(Http::LowerCaseString("cache-hit-status"), "cache_hit"); };

            dec_cb_->sendLocalReply(
              Http::Code::OK,
              absl::string_view{entry.body},
              add_hitStatus_,
              absl::nullopt,
              "cache_hit");

            // TODO: extend TTL of cached response

            return Http::FilterHeadersStatus::StopIteration;
          }

          // cache miss, pass through to upstream
          should_cache_ = true;
          return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterDataStatus RingCacheFilter::decodeData(Buffer::Instance&, bool)
        {
          return Http::FilterDataStatus::Continue;
        }

        Http::FilterTrailersStatus RingCacheFilter::decodeTrailers(Http::RequestTrailerMap&)
        {
          return Http::FilterTrailersStatus::Continue;
        }

        void RingCacheFilter::setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& cb)
        {
          dec_cb_ = &cb;
        }


        // Encoder
        Http::FilterHeadersStatus RingCacheFilter::encodeHeaders(Http::ResponseHeaderMap& headers, bool end_stream)
        {
#ifndef NDEBUG
          ENVOY_LOG(debug,"encodeHeaders on request: [status={}, end_stream={}]",
                   headers.getStatusValue(), end_stream);
#endif

          // for now dont cache nonEndStream requests
          (void)end_stream;

          headers.addCopy(Http::LowerCaseString("cache-hit-status"), "cache_miss");
          return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterDataStatus RingCacheFilter::encodeData(Buffer::Instance&, bool end_stream)
        {
          // TODO: append to ring buffer, fanout to coalesced waiters

          (void)end_stream;
          return Http::FilterDataStatus::Continue;
        }

        Http::FilterTrailersStatus RingCacheFilter::encodeTrailers(Http::ResponseTrailerMap&)
        {
          // TODO: finalize entry, set freshness, notify waiters
          return Http::FilterTrailersStatus::Continue;
        }

        void RingCacheFilter::setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& cb)
        {
          enc_cb_ = &cb;
        }

        void RingCacheFilter::onDestroy()
        {
          // TODO: cleanup/reset if leader aborted; wake waiters with error if needed
        }

        Http::Filter1xxHeadersStatus RingCacheFilter::encode1xxHeaders(Http::ResponseHeaderMap&)
        {
          return Http::Filter1xxHeadersStatus::Continue;
        }

        Http::FilterMetadataStatus RingCacheFilter::encodeMetadata(Http::MetadataMap&)
        {
          return Http::FilterMetadataStatus::Continue;
        }

      } // namespace RingCache
    } // namespace HttpFilters
  } // namespace Extensions
} // namespace Envoy
