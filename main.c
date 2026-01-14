#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BIN_COUNT 10
#define BIN_SIZE 32
#define PAGE_SIZE 4096

typedef struct Chunk
{
    int size;
    struct Chunk *next;
} Chunk;

typedef struct
{
    Chunk *bins[BIN_COUNT];
    void *arena_start;
    int arena_size;
    int arena_offset;
} Allocator;

Allocator allocator = {{NULL}, NULL, 0, 0};

void *request_new_pages(int size)
{
    size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
    void *ptr = sbrk(size);
    if (ptr == (void *)-1)
        return NULL;
    return ptr;
}

int initialize_arena(int arena_size)
{
    allocator.arena_start = malloc(arena_size);
    if (!allocator.arena_start)
        return -1;
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

void custom_free(void *ptr)
{
    if (!ptr)
        return;

    Chunk *chunk = (Chunk *)ptr - 1;
    int bin_index = chunk->size / BIN_SIZE;
    if (bin_index >= BIN_COUNT)
        return;
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
