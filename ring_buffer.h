//
// Created by Lukáš Blažek on 26.08.2025.
//

#ifndef ENVOY_RING_BUFFER_FILTER_RING_BUFFER_H
#define ENVOY_RING_BUFFER_FILTER_RING_BUFFER_H

#include <cstddef>
#include <string>

#include "source/common/http/utility.h"

namespace Envoy::Extensions::HttpFilters::RingCache
{
	struct CacheKey
	{
		std::string host;
		std::string path;

		bool operator==(const CacheKey& other) const
		{
			return host == other.host && path == other.path;
		}

		bool operator<(const CacheKey& other) const
		{
			if (host < other.host) return true;
			if (host > other.host) return false;
			return path < other.path;
		}
	};

	struct CachedResponse
	{
		std::string body;
		Http::ResponseHeaderMapPtr headers;
		Http::ResponseTrailerMapPtr trailers; // might be needed later?
		Http::Code status;
	};

	struct Entry
	{
		CacheKey key;
		CachedResponse value;
	};

	class RingBuffer
	{
	public:
		explicit RingBuffer(size_t cap)
			: cap_(cap) { buf_.resize(cap); }

		// nullptr if not found
		const CachedResponse* find(const CacheKey& key) const;

		void put(CacheKey key, CachedResponse value);

		size_t capacity() const { return cap_; }
		size_t size() const { return size_; }

	private:
		size_t cap_{0};
		size_t size_{0};
		size_t write_{0};
		std::vector<Entry> buf_;
	};

	struct InFlight
	{
		std::vector<Http::StreamDecoderFilterCallbacks*> waiters;
	};

	struct Pool
	{
		std::string name;
		std::vector<std::string> prefixes;
		RingBuffer buffer;

		std::map<CacheKey, InFlight> inflight;

		Pool(std::string n, std::vector<std::string> pfx, size_t slots)
			: name(std::move(n)), prefixes(std::move(pfx)), buffer(slots)
		{
		}

		bool matches(absl::string_view path) const;
	};
} // namespace Envoy::Extensions::HttpFilters::RingCache

#endif //ENVOY_RING_BUFFER_FILTER_RING_BUFFER_H
