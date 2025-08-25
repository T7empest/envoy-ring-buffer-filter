#include "ring_cache.h"

#include "source/common/http/header_map_impl.h"

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
              entry->status,
              absl::string_view{entry->body},
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

          if (!should_cache_)
            return Http::FilterHeadersStatus::Continue;

          // cache the response:
          // headers
          pending_entry_ = std::make_unique<CachedResponse>();
          pending_entry_->headers = Http::ResponseHeaderMapImpl::create();
          Http::ResponseHeaderMapImpl::copyFrom(*pending_entry_->headers, headers);

          // status code
          uint32_t codeNum = 200;
          if (!absl::SimpleAtoi(headers.getStatusValue(), &codeNum))
            codeNum = 200; // fallback
          pending_entry_->status = static_cast<Http::Code>(codeNum);

          if (end_stream)
          {
            ENVOY_LOG(debug, "endstream, serving header only response");
            sharedCache_->cache.emplace(cache_key_, std::move(pending_entry_));
            pending_entry_.reset();
          }

          headers.addCopy(Http::LowerCaseString("cache-hit-status"), "cache_miss");
          return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterDataStatus RingCacheFilter::encodeData(Buffer::Instance& data, bool end_stream)
        {
          if (!should_cache_ || !pending_entry_)
            return Http::FilterDataStatus::Continue;

          // add body to the cached response
          pending_entry_->body = data.toString();

          ENVOY_LOG(debug, "pending entries body: {}", pending_entry_->body);
          ENVOY_LOG(debug, "endstream: {}", end_stream);

          // put in cache
          sharedCache_->cache.emplace(cache_key_, std::move(pending_entry_));
          pending_entry_.reset();

          return Http::FilterDataStatus::Continue;
        }

        Http::FilterTrailersStatus RingCacheFilter::encodeTrailers(Http::ResponseTrailerMap&)
        {
          return Http::FilterTrailersStatus::Continue;
        }

        void RingCacheFilter::setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& cb)
        {
          enc_cb_ = &cb;
        }

        void RingCacheFilter::onDestroy()
        {
          // TODO:
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
