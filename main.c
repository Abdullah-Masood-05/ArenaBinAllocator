/*
 * main.c - ArenaBinAllocator usage demonstration
 *
 * Exercises custom_malloc / custom_free / custom_realloc / merge_free_blocks
 * with basic assertions to verify correct behaviour.
 *
 * Build:  make          (see Makefile)
 * Run:    ./allocator
 */

/* windows.h must come before allocator.h: winioctl.h declares a BIN_COUNT
 * type that our BIN_COUNT macro would otherwise clobber. */
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h> /* SetConsoleOutputCP */
#endif

#include "allocator.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/* ── Helpers ──────────────────────────────────────────────────────────── */

static void print_banner(const char *label)
{
    printf("\n── %s ──\n", label);
}

/* ── Test cases ───────────────────────────────────────────────────────── */

static void test_basic_alloc_free(void)
{
    print_banner("Basic alloc / free");

    void *p1 = custom_malloc(50);
    void *p2 = custom_malloc(100);
    void *p3 = custom_malloc(50);

    assert(p1 && "custom_malloc(50) returned NULL");
    assert(p2 && "custom_malloc(100) returned NULL");
    assert(p3 && "custom_malloc(50) returned NULL");

    printf("  p1=%p  p2=%p  p3=%p\n", p1, p2, p3);

    custom_free(p2);
    custom_free(p1);
    custom_free(p3);

    printf("  All freed successfully.\n");
}

static void test_realloc(void)
{
    print_banner("Realloc");

    void *p = custom_malloc(32);
    assert(p && "initial custom_malloc failed");

    /* Write a known pattern before reallocating. */
    memset(p, 0xAB, 32);

    void *p2 = custom_realloc(p, 128);
    assert(p2 && "custom_realloc failed");

    /* The first 32 bytes must be preserved. */
    unsigned char *bytes = (unsigned char *)p2;
    for (int i = 0; i < 32; i++)
        assert(bytes[i] == 0xAB && "data corrupted by realloc");

    printf("  Data integrity verified after realloc(32 -> 128).\n");

    custom_free(p2);
}

static void test_bin_recycling(void)
{
    print_banner("Bin recycling");

    void *a = custom_malloc(16);
    assert(a);
    custom_free(a);

    /* A fresh allocation of the same size class should reuse the bin slot. */
    void *b = custom_malloc(16);
    assert(b);
    printf("  a=%p  b=%p  (same address = bin reused: %s)\n",
           a, b, a == b ? "yes" : "no – new arena slot");

    custom_free(b);
}

static void test_merge_free_blocks(void)
{
    print_banner("Merge free blocks");

    void *p1 = custom_malloc(32);
    void *p2 = custom_malloc(32);
    assert(p1 && p2);

    custom_free(p1);
    custom_free(p2);
    merge_free_blocks();

    printf("  merge_free_blocks() completed without error.\n");
}

static void test_null_and_edge_cases(void)
{
    print_banner("NULL / edge-case safety");

    /* malloc(0) must return NULL without crashing. */
    void *p = custom_malloc(0);
    assert(p == NULL && "custom_malloc(0) should return NULL");

    /* free(NULL) must be a no-op. */
    custom_free(NULL);

    /* realloc(NULL, n) must behave like malloc(n). */
    void *q = custom_realloc(NULL, 64);
    assert(q && "custom_realloc(NULL, 64) failed");
    custom_free(q);

    printf("  All edge cases passed.\n");
}

/* ── Entry point ──────────────────────────────────────────────────────── */

int main(void)
{
#ifdef _WIN32
    /* The source (and thus all string literals) is UTF-8; make the console
     * decode it as such instead of the legacy OEM code page. */
    SetConsoleOutputCP(CP_UTF8);
#endif

    printf("ArenaBinAllocator – demonstration\n");
    printf("==================================\n");

    if (initialize_arena(16 * PAGE_SIZE) != 0)
    {
        fprintf(stderr, "Failed to initialize memory arena.\n");
        return 1;
    }

    test_basic_alloc_free();
    test_bin_recycling();
    test_realloc();
    test_merge_free_blocks();
    test_null_and_edge_cases();

    printf("\nAll tests passed.\n");
    return 0;
}
