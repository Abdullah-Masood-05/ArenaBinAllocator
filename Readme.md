#  ArenaBinAllocator

**ArenaBinAllocator** is a custom memory allocator implementation written in C that simulates the allocation and deallocation of memory using a pool of memory (arena). It provides basic memory management operations such as `malloc`, `free`, and `realloc` using a bin-based system with a custom memory arena. The implementation is optimized for small allocations by managing free blocks of memory in predefined bins.

## Features

- **Custom Memory Allocation**: Implements `custom_malloc` for allocating memory, which handles binning and larger memory requests through an arena.
- **Memory Deallocation**: Implements `custom_free` to deallocate memory and return it to the appropriate bin.
- **Memory Reallocation**: Implements `custom_realloc` to resize allocated memory, copying data from the old memory block to the new one if necessary.
- **Free Block Merging**: Implements `merge_free_blocks` to merge adjacent free blocks to reduce fragmentation.

## File Structure

- **`allocator.c`**: The implementation of the custom memory allocator, including memory allocation, deallocation, and merging of free blocks.
- **`allocator.h`**: Header file that defines the data structures and function prototypes for the allocator.

## How it Works

1. **Arena Initialization**: The allocator initializes a pool of memory (arena) with a specified size. The arena is divided into chunks of varying sizes for allocations.
2. **Bins for Small Allocations**: Small memory allocations are placed into bins for efficient reuse. Each bin corresponds to a predefined chunk size.
3. **Handling Large Allocations**: For larger allocations, the allocator requests more memory from the system via `sbrk` and places the chunks directly into the arena.
4. **Memory Merging**: When blocks are freed, adjacent free blocks are merged to reduce fragmentation.

## Example Usage

```c
#include "allocator.h"

int main() {
    if (initialize_arena(16 * PAGE_SIZE) == -1) {
        printf("Failed to initialize memory arena.\n");
        return 1;
    }

    // Allocate memory
    void *ptr1 = custom_malloc(50);
    void *ptr2 = custom_malloc(100);
    void *ptr3 = custom_malloc(50);

    // Free some memory
    custom_free(ptr2);
    custom_free(ptr1);
    custom_free(ptr3);

    // Allocate a larger block
    void *ptr4 = custom_malloc(150);

    // Free all memory and merge free blocks
    custom_free(ptr4);
    merge_free_blocks();

    return 0;
}
