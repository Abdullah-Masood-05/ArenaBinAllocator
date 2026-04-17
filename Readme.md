# ArenaBinAllocator

A custom memory allocator written in C that implements `malloc`, `free`, and `realloc` semantics using a **hybrid arena + bin strategy**:

- **Bin-based recycling** for small allocations (≤ 320 bytes) — O(1) free-list per size class.
- **Arena bump-pointer** for large allocations — linear carving from a contiguous region grown via `sbrk(2)`.
- **Free-block coalescing** to reduce fragmentation over time.

> **Platform:** Linux / POSIX only (`sbrk` is used for heap management).

---

## File Structure

| File | Purpose |
|------|---------|
| `allocator.h` | Public API — structs, constants, and function prototypes |
| `allocator.c` | Implementation of the allocator |
| `main.c` | Usage demonstration and basic correctness tests |
| `Makefile` | Build system |

---

## How It Works

### 1. Arena Initialization
`initialize_arena(size)` bootstraps the allocator by requesting a page-aligned memory region from the OS via `sbrk(2)`. All subsequent allocations are served from this region until it is exhausted, at which point a new page-aligned region is requested automatically.

### 2. Bin-Based Small Allocations
Allocations up to `BIN_COUNT × BIN_SIZE` bytes (10 × 32 = **320 bytes**) are routed to one of 10 size-class free-lists. When a chunk is freed, it is prepended to its bin's list and reused immediately by the next matching allocation — this avoids touching the arena again.

```
Size range        Bin index
-----------       ---------
 1  –  32 bytes     0
33  –  64 bytes     1
65  –  96 bytes     2
  …               …
289 – 320 bytes     9
```

### 3. Large Allocations
Requests above 320 bytes are carved directly from the arena. A `Chunk` header is written immediately before the returned pointer, storing the payload size for use by `custom_free` and `custom_realloc`.

### 4. Free Block Merging
`merge_free_blocks()` scans every bin free-list for physically contiguous chunk pairs and merges them. This reduces fragmentation when many same-sized blocks are freed and then needed in larger sizes.

---

## Build & Run

Requires `gcc` and `make` on Linux.

```bash
# Clone
git clone https://github.com/<your-username>/MALLOC.git
cd MALLOC

# Build release binary
make

# Run the demonstration
make run

# Build with AddressSanitizer + UBSan (recommended for development)
make debug
./allocator

# Clean build artefacts
make clean
```

---

## Example Usage

```c
#include "allocator.h"
#include <stdio.h>

int main(void)
{
    /* Initialise a 64 KiB arena */
    if (initialize_arena(16 * PAGE_SIZE) != 0)
    {
        fprintf(stderr, "Failed to initialise arena.\n");
        return 1;
    }

    void *ptr1 = custom_malloc(50);   /* served from bin[1] */
    void *ptr2 = custom_malloc(400);  /* served from arena  */
    void *ptr3 = custom_malloc(50);   /* served from bin[1] */

    custom_free(ptr1);
    custom_free(ptr3);

    /* Reallocate ptr2 from 400 -> 800 bytes */
    ptr2 = custom_realloc(ptr2, 800);

    custom_free(ptr2);
    merge_free_blocks();

    return 0;
}
```

---

## API Reference

### `int initialize_arena(size_t arena_size)`
Initialises the allocator with a heap region of at least `arena_size` bytes. Must be called once before any other function. Returns `0` on success, `-1` on failure.

### `void *custom_malloc(size_t size)`
Allocates `size` bytes. Returns `NULL` if `size == 0` or on allocation failure.

### `void custom_free(void *ptr)`
Frees a previously allocated block. No-op when `ptr` is `NULL`.

### `void *custom_realloc(void *ptr, size_t size)`
Resizes the allocation at `ptr` to `size` bytes, preserving existing data. Behaves like `custom_malloc(size)` when `ptr` is `NULL`, and like `custom_free(ptr)` when `size` is `0`.

### `void merge_free_blocks(void)`
Coalesces adjacent free chunks within each bin to reduce fragmentation.

---

## Known Limitations

- Large allocations are not reclaimed on `custom_free` — `sbrk` does not support arbitrary frees. A production implementation would use `mmap` / `munmap` instead.
- The allocator is **not thread-safe**. Concurrent use requires external locking.
- `initialize_arena` may only be called once per process lifetime in the current implementation.

---

## License

This project is released under the [MIT License](LICENSE).
