// SPDX-FileCopyrightText: 2024-present American Society Of Cinematographers
// SPDX-License-Identifier: Apache-2.0
/**
 * @file fdl_tl_cache.h
 * @brief Bounded thread-local string cache with automatic eviction.
 *
 * Provides a simple size-bounded wrapper around std::unordered_map that
 * auto-clears when the entry count exceeds a threshold.  This prevents
 * unbounded memory growth in long-running threads that access many
 * different documents or nodes.
 *
 * The eviction strategy is a full clear (rather than LRU) because:
 * - Entries are only string caches, not correctness-critical data.
 * - The cost of repopulation is a single string copy per subsequent access.
 * - A full clear is cheaper than LRU bookkeeping for these small strings.
 */
#ifndef FDL_TL_CACHE_H
#define FDL_TL_CACHE_H

#include <deque>
#include <string>
#include <unordered_map>

#include "fdl_constants.h"

namespace fdl::detail {

/**
 * @brief Bounded thread-local string cache with FIFO single-entry eviction.
 *
 * When inserting a new key at @c fdl::constants::kTlCacheMaxEntries capacity,
 * only the oldest cached entry is evicted so recently returned pointers for
 * other keys remain valid (leveraging std::unordered_map reference stability).
 *
 * @tparam Key   Cache key type (must be hashable and equality-comparable).
 * @tparam Hash  Hash functor for @p Key.
 */
template<typename Key, typename Hash> class TlStringCache {
public:
    /**
     * @brief Get or create a cached string entry for the given key.
     *
     * If inserting a new key would exceed the size limit, the oldest entry is
     * evicted first. Existing entries for other keys remain valid until they
     * are themselves evicted.
     *
     * @param key  The cache key to look up.
     * @return Reference to the cached string (may be empty if newly created).
     */
    std::string& get(const Key& key) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            return it->second;
        }
        if (map_.size() >= fdl::constants::kTlCacheMaxEntries && !order_.empty()) {
            map_.erase(order_.front());
            order_.pop_front();
        }
        order_.push_back(key);
        return map_[key];
    }

private:
    std::unordered_map<Key, std::string, Hash> map_;
    std::deque<Key> order_;
};

} // namespace fdl::detail

#endif // FDL_TL_CACHE_H
