//
// Created by Lukáš Blažek on 26.08.2025.
//

#include "ring_buffer.h"

namespace Envoy::Extensions::HttpFilters::RingCache
{
	const CachedResponse* RingBuffer::find(const CacheKey& key) const
	{
		if (size_ == 0 || cap_ == 0)
			return nullptr;

		for (size_t i = 0; i < size_; ++i)
		{
			const size_t index = (write_ + cap_ - 1 - i) % cap_;
			const Entry& entry = buf_[index];
			if (entry.key == key)
				return &entry.value;
		}
		return nullptr;
	}

	void RingBuffer::put(CacheKey key, CachedResponse value)
	{
		if (cap_ == 0)
			return;

		buf_[write_] = Entry{std::move(key), std::move(value)};
		write_ = (write_ + 1) % cap_;

		if (size_ < cap_)
			++size_;
	}

	bool Pool::matches(absl::string_view path) const
	{
		for (const auto& p : prefixes)
		{
			if (path.substr(0, p.size()) == p)
				return true;
		}
		return false;
	}
} // namespace Envoy::Extensions::HttpFilters::RingCache
