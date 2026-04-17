/*
 * ArenaBinAllocator - A custom memory allocator using arena and bin-based strategy
 *
 * This implementation provides malloc/free/realloc functionality with:
 * - Bin-based allocation for small chunks
 * - Arena-based management for large allocations
 * - Free block merging to reduce fragmentation
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BIN_COUNT 10
#define BIN_SIZE 32
#define PAGE_SIZE 4096

typedef struct Chunk
{
    size_t size;
    struct Chunk *next;
} Chunk;

typedef struct
{
    Chunk *bins[BIN_COUNT];
    void *arena_start;
    size_t arena_size;
    size_t arena_offset;
} Allocator;

static Allocator allocator = {{NULL}, NULL, 0, 0};

/**
 * request_new_pages - Request a new page-aligned memory block from the system
 * @size: Requested size in bytes
 *
 * Returns: Pointer to allocated memory or NULL on failure
 */
void *request_new_pages(size_t size)
{
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    void *ptr = sbrk(size);
    if (ptr == (void *)-1)
        return NULL;
    return ptr;
}

/**
 * initialize_arena - Initialize the memory arena
 * @arena_size: Size of the arena to allocate
 *
 * Returns: 0 on success, -1 on failure
 */
int initialize_arena(size_t arena_size)
{
    if (arena_size == 0)
        return -1;
    /**
     * custom_malloc - Allocate memory using bins for small sizes
     * @size: Size of memory to allocate
     *
     * Returns: Pointer to allocated memory or NULL on failure
     *
     * Uses bin-based allocation for chunks <= BIN_COUNT * BIN_SIZE,
     * otherwise uses arena with dynamic page allocation
     */
    void *custom_malloc(size_t size)
    {
        if (size == 0)
            return NULL;

        if (!allocator.arena_start)
            return NULL;

        size_t bin_index = size / BIN_SIZE;

        allocator.arena_size = arena_size;
        allocator.arena_offset = 0;
        return 0;
    }

    void *custom_malloc(int size)
    {
        if (size == 0)
            return NULL;
        int bin_index = size / BIN_SIZE;
        if (bin_index >= BIN_COUNT)
        {
            if (allocator.arena_offset + sizeof(Chunk) + size > allocator.arena_size)
            {
                void *new_pages = request_new_pages(PAGE_SIZE);
                if (!new_pages)
                    return NULL;
                allocator.arena_start = new_pages;
                allocator.arena_offset = sizeof(Chunk);
            }
            Chunk *chunk = (Chunk *)(allocator.arena_start + allocator.arena_offset);
            chunk->size = size;
            allocator.arena_offset += sizeof(Chunk) + size;
            return (void *)(chunk + 1);
        }

        if (allocator.bins[bin_index] != NULL)
        {
            Chunk *chunk = allocator.bins[bin_index];
            allocator.bins[bin_index] = chunk->next;
            return (void *)(chunk + 1);
        }

        /**
         * custom_free - Deallocate memory and return to bin for reuse
         * @ptr: Pointer to memory to deallocate
         *
         * Returns: void
         *
         * Only bins managed chunks; large allocations are not freed
         */
        void custom_free(void *ptr)
        {
            if (!ptr)
                return;

            /**
             * custom_realloc - Resize allocated memory block
             * @ptr: Pointer to existing allocation (or NULL)
             * @size: New size in bytes
             *
             * Returns: Pointer to resized memory or NULL on failure
             *
             * If size is smaller than current, returns the same pointer.
             * If larger, allocates new block and copies data.
             */
            void *custom_realloc(void *ptr, size_t size)
            {
                if (ptr == NULL)
                    return custom_malloc(size);

                Chunk *chunk = (Chunk *)ptr - 1;
                size_locator.arena_offset += sizeof(Chunk) + size;
                return (void *)(chunk + 1);
            }
            /**
             * merge_free_blocks - Merge adjacent free blocks to reduce fragmentation
             *
             * Returns: void
             *
             * Iterates through all bins and merges contiguous free chunks
             */
            void merge_free_blocks(void)
            {
                for (size_t i = 0; i < BIN_COUNT; i++)
                {
                    Chunk *current = allocator.bins[i];
                    while (current != NULL && current->next != NULL)
                    {
                        if ((char *)current + sizeof(Chunk) + current->size == (char *)current->next)
                        {
                            current->size += sizeof(Chunk) +
                                             chunk->next = allocator.bins[bin_index];
                            allocator.bins[bin_index] = chunk;
                        }

                        void *custom_realloc(void *ptr, int size)
                        {
                            if (ptr == NULL)
                                return custom_malloc(size);

                            Chunk *chunk = (Chunk *)ptr - 1;
                            int old_size = chunk->size;
                            if (old_size >= size)
                                return ptr;

                            void *new_ptr = custom_malloc(size);
                            if (new_ptr)
                            {
                                memcpy(new_ptr, ptr, old_size);
                                custom_free(ptr);
                            }
                            return new_ptr;
                        }

                        void merge_free_blocks()
                        {
                            for (int i = 0; i < BIN_COUNT; i++)
                            {
                                Chunk *current = allocator.bins[i];
                                while (current != NULL && current->next != NULL)
                                {
                                    if ((char *)current + current->size == (char *)current->next)
                                    {
                                        current->size += current->next->size;
                                        current->next = current->next->next;
                                    }
                                    else
                                        current = current->next;
                                }
                            }
                        }
