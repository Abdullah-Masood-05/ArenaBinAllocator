/*
 * allocator.h - ArenaBinAllocator public interface
 *
 * Defines the data structures, constants, and function prototypes
 * for the custom arena/bin-based memory allocator.
 */

#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

/* ── Constants ─────────────────────────────────────────────────────────── */

/**
 * BIN_COUNT - Number of size-class bins.
 * Each bin i holds free chunks in the range (i * BIN_SIZE, (i+1) * BIN_SIZE].
 */
#define BIN_COUNT 10

/**
 * BIN_SIZE - Granularity (bytes) of each bin size class.
 * Allocations <= BIN_COUNT * BIN_SIZE are served from bins.
 */
#define BIN_SIZE 32

/** PAGE_SIZE - System page size used for arena growth. */
#define PAGE_SIZE 4096

/* ── Data Structures ───────────────────────────────────────────────────── */

/**
 * struct Chunk - Metadata header stored immediately before every allocation.
 * @size: Usable payload size in bytes (does not include sizeof(Chunk)).
 * @next: Intrusive linked-list pointer used when the chunk is in a free bin.
 */
typedef struct Chunk
{
    size_t       size;
    struct Chunk *next;
} Chunk;

/**
 * struct Allocator - Global allocator state.
 * @bins:         Free-lists for each size class (indexed by size / BIN_SIZE).
 * @arena_start:  Base address of the current arena region.
 * @arena_size:   Total capacity of the current arena region in bytes.
 * @arena_offset: Bytes already consumed within the current arena region.
 */
typedef struct
{
    Chunk *bins[BIN_COUNT];
    void  *arena_start;
    size_t arena_size;
    size_t arena_offset;
} Allocator;

/* ── Public API ────────────────────────────────────────────────────────── */

/**
 * initialize_arena - Set up the memory arena before any allocations.
 * @arena_size: Desired initial arena capacity in bytes (must be > 0).
 *
 * Returns 0 on success, -1 on failure (invalid size or out of memory).
 * Must be called exactly once before custom_malloc / custom_free / custom_realloc.
 */
int initialize_arena(size_t arena_size);

/**
 * custom_malloc - Allocate @size bytes of memory.
 * @size: Number of bytes to allocate.
 *
 * Returns a pointer to the usable memory region, or NULL on failure.
 * Small allocations (<= BIN_COUNT * BIN_SIZE) are served from the bin
 * free-lists; larger allocations are carved directly from the arena,
 * growing it via sbrk() if necessary.
 */
void *custom_malloc(size_t size);

/**
 * custom_free - Return the allocation at @ptr to the allocator.
 * @ptr: Pointer previously returned by custom_malloc / custom_realloc,
 *       or NULL (in which case the call is a no-op).
 *
 * Small-chunk pointers are added to the appropriate bin free-list for
 * reuse.  Large chunks are currently not reclaimed (sbrk does not support
 * arbitrary frees).
 */
void custom_free(void *ptr);

/**
 * custom_realloc - Resize the allocation at @ptr to @size bytes.
 * @ptr:  Pointer previously returned by custom_malloc / custom_realloc,
 *        or NULL (equivalent to custom_malloc(@size)).
 * @size: New desired size in bytes.
 *
 * Returns a pointer to the (possibly moved) allocation, or NULL on failure.
 * If the existing allocation is already large enough, the original pointer
 * is returned unchanged.  Otherwise a new block is allocated, the data is
 * copied, and the old block is freed.
 */
void *custom_realloc(void *ptr, size_t size);

/**
 * merge_free_blocks - Coalesce physically adjacent free chunks within bins.
 *
 * Iterates every bin free-list and merges contiguous chunk pairs, reducing
 * heap fragmentation.  Call periodically or after a batch of frees.
 */
void merge_free_blocks(void);

#endif /* ALLOCATOR_H */
