# Caching

## Why cache
Reduce latency and load on the primary data store.

## Where to cache
Client · CDN · API gateway · Application (local) · Distributed (Redis/Memcached) · Database

## Write strategies
- Cache-aside
- Write-through
- Write-behind

## Eviction
LRU · LFU · FIFO · TTL

## Pitfalls
Stale data, cache stampede, hot keys, cold start
