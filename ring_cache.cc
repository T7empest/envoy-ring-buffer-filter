#include "ring_cache.h"

namespace Envoy {
  namespace Extensions {
    namespace HttpFilters {
      namespace RingCache {

        // ---- Decoder ----
        Http::FilterHeadersStatus RingCacheFilter::decodeHeaders(Http::RequestHeaderMap&, bool) {
          // TODO: build cache key, lookup/coalesce, maybe short-circuit
          return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterDataStatus RingCacheFilter::decodeData(Buffer::Instance&, bool) {
          // TODO: request body handling (rarely needed for GET)
          return Http::FilterDataStatus::Continue;
        }

        Http::FilterTrailersStatus RingCacheFilter::decodeTrailers(Http::RequestTrailerMap&) {
          return Http::FilterTrailersStatus::Continue;
        }

        void RingCacheFilter::setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& cb) {
          dec_cb_ = &cb;
        }

        // ---- Encoder ----
        Http::FilterHeadersStatus RingCacheFilter::encodeHeaders(Http::ResponseHeaderMap&, bool) {
          // TODO: cacheability decision, prepare entry
          return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterDataStatus RingCacheFilter::encodeData(Buffer::Instance&, bool end_stream) {
          // TODO: append to ring buffer, fanout to coalesced waiters
          (void)end_stream;
          return Http::FilterDataStatus::Continue;
        }

        Http::FilterTrailersStatus RingCacheFilter::encodeTrailers(Http::ResponseTrailerMap&) {
          // TODO: finalize entry, set freshness, notify waiters
          return Http::FilterTrailersStatus::Continue;
        }

        void RingCacheFilter::setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& cb) {
          enc_cb_ = &cb;
        }

        void RingCacheFilter::onDestroy() {
          // TODO: cleanup/reset if leader aborted; wake waiters with error if needed
        }

        Http::Filter1xxHeadersStatus RingCacheFilter::encode1xxHeaders(Http::ResponseHeaderMap&) {
          return Http::Filter1xxHeadersStatus::Continue;
        }

        Http::FilterMetadataStatus RingCacheFilter::encodeMetadata(Http::MetadataMap&) {
          return Http::FilterMetadataStatus::Continue;
        }


      } // namespace RingCache
    } // namespace HttpFilters
  } // namespace Extensions
} // namespace Envoy
