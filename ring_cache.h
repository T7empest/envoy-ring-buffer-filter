#pragma once

#include <utility>

#include "ring_buffer.h"
#include "envoy/http/filter.h"
#include "source/common/common/logger.h"

namespace Envoy::Extensions::HttpFilters::RingCache
{
	class RingCacheFilter : public Http::StreamFilter,
	                        public Logger::Loggable<Logger::Id::filter>
	{
	public:
		explicit RingCacheFilter(std::shared_ptr<std::vector<Pool>> pools) : pools_(
			std::move(pools))
		{
		}

		// Decoder (request → upstream)
		Http::FilterHeadersStatus decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream) override;
		Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override;
		Http::FilterTrailersStatus decodeTrailers(Http::RequestTrailerMap& trailers) override;
		void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override;

		// Encoder (upstream → client)
		Http::FilterHeadersStatus encodeHeaders(Http::ResponseHeaderMap& headers, bool end_stream) override;
		Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override;
		Http::FilterTrailersStatus encodeTrailers(Http::ResponseTrailerMap& trailers) override;
		void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override;

		// extra encoder hooks required by Envoy API
		Http::Filter1xxHeadersStatus encode1xxHeaders(Http::ResponseHeaderMap& headers) override;
		Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap& metadata_map) override;

		void onDestroy() override;

	private:
		// pick a pool for given path
		Pool* pickPool(absl::string_view path) const
		{
			if (!pools_)
				return nullptr;
			for (auto& p : *pools_)
			{
				if (p.matches(path)) return &p;
			}
			return nullptr;
		}

		std::shared_ptr<std::vector<Pool>> pools_;
		Http::StreamDecoderFilterCallbacks* dec_cb_{nullptr};
		Http::StreamEncoderFilterCallbacks* enc_cb_{nullptr};

		Pool* selected_pool_{nullptr};
		CacheKey pending_key_;
		std::unique_ptr<CachedResponse> pending_entry_;
		bool should_cache_{false};

		bool is_leader_{false};
		bool is_follower_{false};
		CacheKey coalesce_key_;
		InFlight* my_inflight_{nullptr};
	};
} // namespace Envoy::Extensions::HttpFilters::RingCache
