/*
 * allocator.c - ArenaBinAllocator implementation
 *
 * Provides custom_malloc / custom_free / custom_realloc / merge_free_blocks
 * using a hybrid arena + bin-based strategy:
 *
 *   - Small allocations (<= BIN_COUNT * BIN_SIZE bytes) are served from
 *     per-size-class free-lists (bins) to enable O(1) recycling.
 *   - Large allocations are carved linearly from an arena region that
 *     grows on demand (sbrk(2) on POSIX, VirtualAlloc on Windows).
 *   - merge_free_blocks() coalesces physically adjacent free chunks to
 *     reduce fragmentation over time.
 */

/* Expose POSIX/GNU extensions (sbrk, intptr_t, etc.) under -std=c11.
 * Must appear before any #include. */
#define _GNU_SOURCE

/* windows.h must come before allocator.h: winioctl.h declares a BIN_COUNT
 * type that our BIN_COUNT macro would otherwise clobber. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* VirtualAlloc      */
#else
#include <unistd.h>  /* sbrk              */
#endif

#include "allocator.h"

#include <stdio.h>   /* fprintf, stderr   */
#include <string.h>  /* memcpy            */

/* ── Module-private state ─────────────────────────────────────────────── */

static Allocator allocator = { { NULL }, NULL, 0, 0 };

/* ── Internal helpers ─────────────────────────────────────────────────── */

/**
 * request_new_pages - Extend the heap by at least @size bytes.
 * @size: Minimum number of bytes required.
 *
 * Uses sbrk(2) on POSIX systems and VirtualAlloc on Windows.  The request
 * is rounded up to the nearest PAGE_SIZE boundary so that the heap pointer
 * always stays page-aligned.
 *
 * Returns a pointer to the newly allocated region, or NULL on failure.
 */
static void *request_new_pages(size_t size)
{
    /* Round up to the next page boundary. */
    size = (size + PAGE_SIZE - 1) & ~((size_t)(PAGE_SIZE - 1));

#ifdef _WIN32
    void *ptr = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (ptr == NULL)
    {
        fprintf(stderr, "allocator: VirtualAlloc failed - out of memory\n");
        return NULL;
    }
    return ptr;
#else
    void *ptr = sbrk((intptr_t)size);
    if (ptr == (void *)-1)
    {
        fprintf(stderr, "allocator: sbrk failed - out of memory\n");
        return NULL;
    }
    return ptr;
#endif
}

/**
 * bin_index_for - Compute the bin index for a given allocation @size.
 * @size: Requested payload size in bytes.
 *
 * Returns the bin index, or BIN_COUNT if @size exceeds the bin range
 * (i.e. it should be served from the arena directly).
 */
static size_t bin_index_for(size_t size)
{
    if (size == 0)
        return 0;
    size_t index = (size - 1) / BIN_SIZE;
    return index < BIN_COUNT ? index : BIN_COUNT;
}

/* ── Public API implementation ────────────────────────────────────────── */

int initialize_arena(size_t arena_size)
{
    if (arena_size == 0)
    {
        fprintf(stderr, "allocator: arena_size must be greater than 0\n");
        return -1;
    }

    void *mem = request_new_pages(arena_size);
    if (!mem)
        return -1;

    allocator.arena_start  = mem;
    allocator.arena_size   = arena_size;
    allocator.arena_offset = 0;

    return 0;
}

void *custom_malloc(size_t size)
{
    if (size == 0)
        return NULL;

    if (!allocator.arena_start)
    {
        fprintf(stderr, "allocator: arena not initialised – call initialize_arena() first\n");
        return NULL;
    }

    size_t index = bin_index_for(size);

    /* ── Small allocation: try the bin free-list first ── */
    if (index < BIN_COUNT)
    {
        if (allocator.bins[index] != NULL)
        {
            Chunk *chunk          = allocator.bins[index];
            allocator.bins[index] = chunk->next;
            chunk->next           = NULL;
            return (void *)(chunk + 1);
        }

        /* Bin is empty – carve a fixed-size slot from the arena.
         * All chunks in bin[i] have the same payload size: (i+1)*BIN_SIZE. */
        size = (index + 1) * BIN_SIZE;
    }

    /* ── Large allocation (or small with empty bin): use the arena ── */
    size_t needed = sizeof(Chunk) + size;

    if (allocator.arena_offset + needed > allocator.arena_size)
    {
        /* Arena exhausted – request another page-aligned region. */
        void *new_pages = request_new_pages(needed > PAGE_SIZE ? needed : PAGE_SIZE);
        if (!new_pages)
            return NULL;

        allocator.arena_start  = new_pages;
        allocator.arena_size   = needed > PAGE_SIZE ? needed : PAGE_SIZE;
        allocator.arena_offset = 0;
    }

    Chunk *chunk = (Chunk *)((char *)allocator.arena_start + allocator.arena_offset);
    chunk->size  = size;
    chunk->next  = NULL;

    allocator.arena_offset += needed;

    return (void *)(chunk + 1);
}

void custom_free(void *ptr)
{
    if (!ptr)
        return;

    Chunk  *chunk = (Chunk *)ptr - 1;
    size_t  index = bin_index_for(chunk->size);

    if (index < BIN_COUNT)
    {
        /* Return the chunk to the head of its bin free-list. */
        chunk->next           = allocator.bins[index];
        allocator.bins[index] = chunk;
    }
    /*
     * Large chunks cannot be returned to the heap via sbrk because sbrk
     * only supports moving the program break in one direction on most
     * kernels.  In a production allocator this would use munmap instead.
     */
}

void *custom_realloc(void *ptr, size_t size)
{
    if (ptr == NULL)
        return custom_malloc(size);

    if (size == 0)
    {
        custom_free(ptr);
        return NULL;
    }

    Chunk  *chunk    = (Chunk *)ptr - 1;
    size_t  old_size = chunk->size;

    /* If the existing chunk already fits, return it unchanged. */
    if (old_size >= size)
        return ptr;

    void *new_ptr = custom_malloc(size);
    if (!new_ptr)
        return NULL;

    memcpy(new_ptr, ptr, old_size);
    custom_free(ptr);

    return new_ptr;
}

void merge_free_blocks(void)
{
    for (size_t i = 0; i < BIN_COUNT; i++)
    {
        Chunk *current = allocator.bins[i];

        while (current != NULL && current->next != NULL)
        {
            /*
             * Two chunks are physically adjacent when the byte immediately
             * after current's payload is the start of current->next's header.
             */
            char *end_of_current = (char *)current + sizeof(Chunk) + current->size;

            if (end_of_current == (char *)current->next)
            {
                /* Absorb current->next into current. */
                current->size += sizeof(Chunk) + current->next->size;
                current->next  = current->next->next;
                /* Do not advance: the newly merged chunk may be adjacent
                 * to the next one too. */
            }
            else
            {
                current = current->next;
            }
        }
    }
}
