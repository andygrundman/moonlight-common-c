#pragma once

#include "PlatformThreads.h"

#include <stdatomic.h>

typedef struct _PACKET_BUFFER_SLOT PACKET_BUFFER_SLOT;

typedef struct _PACKET_BUFFER_POOL_STATS {
    size_t capacity;
    size_t inUse;
    size_t highWaterMark;
    uint64_t heapFallbacks;
} PACKET_BUFFER_POOL_STATS;

// Internal state for one independently sized stream pool. Zero-initialize before
// first use and do not copy a live instance. Initialize/destroy only while all
// its users are stopped.
typedef struct _PACKET_BUFFER_POOL {
    void* allocation;
    char* slab;
    PACKET_BUFFER_SLOT* freeHead;
    size_t bufferSize;
    size_t capacity;
    size_t stride;
    size_t inUse;
    size_t highWaterMark;
    atomic_ullong heapFallbacks;
    PLT_MUTEX mutex;
    bool synchronized;
    const char* name;
} PACKET_BUFFER_POOL;

// name must outlive the pool. singleThreaded also requires cleanup and stats
// access to be serialized with the stream's receive thread.
void PacketBufferPoolInitialize(PACKET_BUFFER_POOL* pool, size_t bufferSize,
                                size_t capacity, bool singleThreaded, const char* name);
void PacketBufferPoolDestroy(PACKET_BUFFER_POOL* pool);

// Requests up to bufferSize use a full slot; larger requests, pool exhaustion,
// and unavailable pools fall back to PltMalloc(). Contents are not cleared.
void* PacketBufferPoolAcquire(PACKET_BUFFER_POOL* pool, size_t size);

// Accepts this pool's buffers and ordinary heap allocations. Buffers from a
// different pool must be returned to that pool, not passed here.
void PacketBufferPoolRelease(PACKET_BUFFER_POOL* pool, void* buffer);
void PacketBufferPoolGetStats(PACKET_BUFFER_POOL* pool, PACKET_BUFFER_POOL_STATS* stats);
