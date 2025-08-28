#include "ring_cache.h"

#include "source/common/http/header_map_impl.h"

namespace Envoy::Extensions::HttpFilters::RingCache
{
	Http::FilterHeadersStatus RingCacheFilter::decodeHeaders(Http::RequestHeaderMap& headers, bool end_stream)
	{
		ENVOY_LOG(
			debug, "decodeHeaders on request: [method={}, path={}, end_stream={}]",
			headers.getMethodValue(), headers.getPathValue(), end_stream);

		// reject caching of non-GET requests
		if (headers.getMethodValue() != "GET")
			return Http::FilterHeadersStatus::Continue;

		// reject caching for streamed requests
		if (!end_stream)
			return Http::FilterHeadersStatus::Continue;

		const absl::string_view path = headers.getPathValue();
		selected_pool_ = pickPool(path);
		if (selected_pool_ == nullptr)
		{
			ENVOY_LOG(debug, "could not pick a pool, given path: {}", path);
			return Http::FilterHeadersStatus::Continue;
		}

		pending_key_ = {
			.host = std::string(headers.getHostValue()),
			.path = std::string(headers.getPathValue())
		};

		// if cache hit -> shortcircuit and serve client directly from cache
		if (const CachedResponse* hit = selected_pool_->buffer.find(pending_key_))
		{
			ENVOY_LOG(debug, "HIT pool: {}, path: {}", selected_pool_->name,
			          pending_key_.path);
			ENVOY_LOG(debug, "SERVING FROM CACHE, body={}", hit->body);

			const auto code = hit->status;
			const std::string& body = hit->body;

			dec_cb_->sendLocalReply(
				code,
				body,
				[](Http::ResponseHeaderMap& h)
				{
					h.addCopy(Http::LowerCaseString("cache-hit-status"), "cache_hit");
				},
				absl::nullopt,
				"cache_hit");

			return Http::FilterHeadersStatus::StopIteration;
		}

		coalesce_key_ = pending_key_;

		auto [it, inserted] = selected_pool_->inflight.emplace(
			coalesce_key_, InFlight{});
		my_inflight_ = &it->second;

		if (inserted)
		{
			ENVOY_LOG(debug, "MISS (Become leader) pool={}, path={}",
			          selected_pool_->name, pending_key_.path);
			is_leader_ = true;
			should_cache_ = true;
			return Http::FilterHeadersStatus::Continue;
		}
		else
		{
			ENVOY_LOG(debug, "MISS (Become follower) pool={}, path={}, followers={}",
			          selected_pool_->name, pending_key_.path,
			          my_inflight_->waiters.size());
			is_follower_ = true;
			my_inflight_->waiters.push_back(dec_cb_);
			return Http::FilterHeadersStatus::StopIteration;
		}
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


	Http::FilterHeadersStatus RingCacheFilter::encodeHeaders(Http::ResponseHeaderMap& headers, bool end_stream)
	{
		ENVOY_LOG(debug, "encodeHeaders on request: [status={}, end_stream={}]",
		          headers.getStatusValue(), end_stream);

		if (!is_leader_ || selected_pool_ == nullptr)
			return Http::FilterHeadersStatus::Continue;

		pending_entry_ = std::make_unique<CachedResponse>();

		pending_entry_->status = static_cast<Http::Code>(
			Http::Utility::getResponseStatus(headers));

		auto hdrs = Http::ResponseHeaderMapImpl::create();
		Http::HeaderMapImpl::copyFrom(*hdrs, headers);
		pending_entry_->headers = std::move(hdrs);

		if (end_stream) // header only response
		{
			ENVOY_LOG(debug, "header only response, caching + fan-out, path={}",
			          coalesce_key_.path);

			for (auto* follower_cb : my_inflight_->waiters)
			{
				follower_cb->sendLocalReply(
					pending_entry_->status,
					"",
					[](Http::ResponseHeaderMap& h)
					{
						h.addCopy(Http::LowerCaseString("cache-hit-status"),
						          "coalesced_hit");
					},
					absl::nullopt,
					"coalesced_hit");
			}

			selected_pool_->buffer.put(std::move(pending_key_),
			                           std::move(*pending_entry_));

			selected_pool_->inflight.erase(coalesce_key_);
			is_leader_ = false;
		}

		headers.addCopy(Http::LowerCaseString("cache-hit-status"), "cache_miss");

		return Http::FilterHeadersStatus::Continue;
	}

	Http::FilterDataStatus RingCacheFilter::encodeData(Buffer::Instance& data, bool end_stream)
	{
		if (!is_leader_)
			return Http::FilterDataStatus::Continue;

		pending_entry_->body.append(data.toString());

		if (end_stream)
		{
			ENVOY_LOG(debug, "leader: body complete; caching + fan-out, path={}",
			          coalesce_key_.path);

			for (auto* follower_cb : my_inflight_->waiters)
			{
				follower_cb->sendLocalReply(
					pending_entry_->status,
					pending_entry_->body,
					[](Http::ResponseHeaderMap& h)
					{
						h.addCopy(Http::LowerCaseString("cache-hit-status"), "coalesced_hit");
					},
					absl::nullopt,
					"coalesced_hit");
			}

			selected_pool_->buffer.put(std::move(pending_key_),
			                           std::move(*pending_entry_));

			selected_pool_->inflight.erase(coalesce_key_);
			is_leader_ = false;
		}

		return Http::FilterDataStatus::Continue;
	}

	Http::FilterTrailersStatus RingCacheFilter::encodeTrailers(Http::ResponseTrailerMap& trailers)
	{
		if (!should_cache_)
			return Http::FilterTrailersStatus::Continue;

		auto trls = Http::ResponseTrailerMapImpl::create();
		Http::ResponseTrailerMapImpl::copyFrom(*trls, trailers);
		pending_entry_->trailers = std::move(trls);
		selected_pool_->buffer.put(std::move(pending_key_),
		                           std::move(*pending_entry_));
		should_cache_ = false;

		return Http::FilterTrailersStatus::Continue;
	}

	void RingCacheFilter::setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& cb)
	{
		enc_cb_ = &cb;
	}

	void RingCacheFilter::onDestroy()
	{
		// if a follower and stream got interrupted remove from inflight table to avoid dangling pointer
		if (is_follower_ && my_inflight_ != nullptr)
		{
			std::erase(my_inflight_->waiters, dec_cb_);
			is_follower_ = false;
		}
	}

	Http::Filter1xxHeadersStatus RingCacheFilter::encode1xxHeaders(Http::ResponseHeaderMap&)
	{
		return Http::Filter1xxHeadersStatus::Continue;
	}

	Http::FilterMetadataStatus RingCacheFilter::encodeMetadata(Http::MetadataMap&)
	{
		return Http::FilterMetadataStatus::Continue;
	}
} // namespace Envoy::Extensions::HttpFilters::RingCache
