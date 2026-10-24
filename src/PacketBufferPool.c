#include "Limelight-internal.h"
#include "PacketBufferPool.h"

#define PACKET_BUFFER_ALIGNMENT 64

// Keep the free-list prefix outside packet and queue-entry storage, since
// consumers may repurpose those regions while retaining the allocation.
struct _PACKET_BUFFER_SLOT {
    struct _PACKET_BUFFER_SLOT* next;
#ifdef LC_DEBUG
    bool inUse;
#endif
};

_Static_assert(sizeof(PACKET_BUFFER_SLOT) <= PACKET_BUFFER_ALIGNMENT, "Pool metadata exceeds slot prefix");

static void lockPool(PACKET_BUFFER_POOL* pool) {
    if (pool->synchronized) {
        PltLockMutex(&pool->mutex);
    }
}

static void unlockPool(PACKET_BUFFER_POOL* pool) {
    if (pool->synchronized) {
        PltUnlockMutex(&pool->mutex);
    }
}

void PacketBufferPoolInitialize(PACKET_BUFFER_POOL* pool, size_t bufferSize,
                                size_t capacity, bool singleThreaded, const char* name) {
    LC_ASSERT(pool->allocation == NULL);
    memset(pool, 0, sizeof(*pool));
    atomic_init(&pool->heapFallbacks, 0);
    pool->bufferSize = bufferSize;
    pool->capacity = capacity;
    pool->name = name != NULL ? name : "Packet";

    // Reserve a full cache line before each buffer and align the stride, keeping
    // both packet data and queue metadata naturally aligned at negotiated sizes.
    if (capacity == 0 || bufferSize == 0 || bufferSize > SIZE_MAX - (2 * PACKET_BUFFER_ALIGNMENT - 1)) {
        Limelog("%s buffer pool disabled: invalid size\n", pool->name);
        return;
    }
    pool->stride = (PACKET_BUFFER_ALIGNMENT + bufferSize + PACKET_BUFFER_ALIGNMENT - 1) &
                  ~(size_t)(PACKET_BUFFER_ALIGNMENT - 1);
    if (pool->stride > (SIZE_MAX - (PACKET_BUFFER_ALIGNMENT - 1)) / pool->capacity) {
        Limelog("%s buffer pool disabled: allocation size overflow\n", pool->name);
        return;
    }
    size_t bytes = pool->stride * pool->capacity + PACKET_BUFFER_ALIGNMENT - 1;

    // This allocation is optional. PltMalloc() aborts on failure in Tracy builds,
    // so allocate directly and track the slab once, rather than each checkout.
    pool->allocation = malloc(bytes);
    if (pool->allocation == NULL) {
        Limelog("%s buffer pool unavailable; using heap buffers\n", pool->name);
        return;
    }
    TracyCAlloc(pool->allocation, bytes);

    if (!singleThreaded) {
        if (PltCreateMutex(&pool->mutex) != 0) {
            PltFree(pool->allocation);
            pool->allocation = NULL;
            Limelog("%s buffer pool mutex unavailable; using heap buffers\n", pool->name);
            return;
        }
        pool->synchronized = true;
    }

    pool->slab = (char*)(((uintptr_t)pool->allocation + PACKET_BUFFER_ALIGNMENT - 1) &
                        ~(uintptr_t)(PACKET_BUFFER_ALIGNMENT - 1));
    for (size_t i = 0; i < pool->capacity; i++) {
        PACKET_BUFFER_SLOT* slot = (PACKET_BUFFER_SLOT*)(pool->slab + i * pool->stride);
        slot->next = pool->freeHead;
#ifdef LC_DEBUG
        slot->inUse = false;
#endif
        pool->freeHead = slot;
    }
    Limelog("%s buffer pool: %zu slots, %zu bytes per slot\n",
            pool->name, pool->capacity, pool->stride);
}

void* PacketBufferPoolAcquire(PACKET_BUFFER_POOL* pool, size_t size) {
    if (pool->slab != NULL && size <= pool->bufferSize) {
        lockPool(pool);
        PACKET_BUFFER_SLOT* slot = pool->freeHead;
        if (slot != NULL) {
            pool->freeHead = slot->next;
#ifdef LC_DEBUG
            LC_ASSERT(!slot->inUse);
            slot->inUse = true;
#endif
            pool->inUse++;
            if (pool->inUse > pool->highWaterMark) {
                pool->highWaterMark = pool->inUse;
            }
        }
        unlockPool(pool);
        if (slot != NULL) {
            return (char*)slot + PACKET_BUFFER_ALIGNMENT;
        }
    }

    atomic_fetch_add_explicit(&pool->heapFallbacks, 1, memory_order_relaxed);
    return PltMalloc(size);
}

void PacketBufferPoolRelease(PACKET_BUFFER_POOL* pool, void* buffer) {
    if (buffer == NULL) {
        return;
    }

    // Integer comparisons avoid undefined pointer comparisons/subtraction when
    // the caller returns an unrelated heap allocation or copied NAL fragment.
    uintptr_t address = (uintptr_t)buffer;
    uintptr_t base = (uintptr_t)pool->slab;
    if (pool->slab != NULL && address >= base &&
            address - base < pool->stride * pool->capacity) {
        LC_ASSERT((address - base) % pool->stride == PACKET_BUFFER_ALIGNMENT);
        PACKET_BUFFER_SLOT* slot = (PACKET_BUFFER_SLOT*)((char*)buffer - PACKET_BUFFER_ALIGNMENT);
        lockPool(pool);
        LC_ASSERT(pool->inUse > 0);
#ifdef LC_DEBUG
        LC_ASSERT(slot->inUse);
        slot->inUse = false;
#endif
        pool->inUse--;
        slot->next = pool->freeHead;
        pool->freeHead = slot;
        unlockPool(pool);
    }
    else {
        PltFree(buffer);
    }
}

void PacketBufferPoolGetStats(PACKET_BUFFER_POOL* pool, PACKET_BUFFER_POOL_STATS* stats) {
    lockPool(pool);
    stats->capacity = pool->slab != NULL ? pool->capacity : 0;
    stats->inUse = pool->inUse;
    stats->highWaterMark = pool->highWaterMark;
    stats->heapFallbacks = atomic_load_explicit(&pool->heapFallbacks, memory_order_relaxed);
    unlockPool(pool);
}

void PacketBufferPoolDestroy(PACKET_BUFFER_POOL* pool) {
    PACKET_BUFFER_POOL_STATS stats;
    PacketBufferPoolGetStats(pool, &stats);
    Limelog("%s buffer pool: peak %zu/%zu slots, %llu heap fallbacks, %zu outstanding\n",
            pool->name, stats.highWaterMark, stats.capacity, (unsigned long long)stats.heapFallbacks, stats.inUse);
    LC_ASSERT(stats.inUse == 0);

    // All packet owners must have been drained before freeing the backing slab.
    if (pool->allocation != NULL) {
        PltFree(pool->allocation);
    }
    if (pool->synchronized) {
        PltDeleteMutex(&pool->mutex);
    }
    memset(pool, 0, sizeof(*pool));
    atomic_init(&pool->heapFallbacks, 0);
}
