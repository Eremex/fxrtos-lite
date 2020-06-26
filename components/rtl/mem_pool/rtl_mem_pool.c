/* Copyright (c) 2018 JSC EREMEX.
 * Copyright (c) 2016 National Cheng Kung University, Taiwan.
 * Copyright (c) 2006-2008, 2011, 2014 Matthew Conte.
 * All rights reserved.
 */

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(HW_CPU)
#include FX_INTERFACE(RTL_MEM_POOL)

FX_METADATA(({ implementation: [RTL_MEM_POOL, TLSF] }))

//
// This code has been tested on 32- and 64-bit (LP/LLP) architectures.
//
lang_static_assert(lang_type_to_bits(int) == 32);
lang_static_assert(lang_type_to_bits(size_t) >= 32);
lang_static_assert(lang_type_to_bits(size_t) <= 64);

//
// SL_INDEX_COUNT must be <= number of bits in sl_bitmap's storage type.
//
lang_static_assert(lang_type_to_bits(unsigned int) >= SL_INDEX_COUNT);

//
// Ensure we've properly tuned our sizes.
//
lang_static_assert(ALIGN_SIZE == SMALL_BLOCK_SIZE / SL_INDEX_COUNT);

//
// Export some additional functions related to pool internals in test builds.
// These functions do not change anything (const objects as an arguments).
//
#ifndef FX_TEST_BUILD
#define STATIC static
#else
#define STATIC extern
#endif

//
// Global constants.
//
enum
{
    //
    // Since block sizes are always at least a multiple of 4, the two least
    // significant bits of the size field are used to store the block status:
    // - bit 0: whether block is busy or free
    // - bit 1: whether previous block is busy or free
    //
    BLK_HEADER_FREE_BIT = 1 << 0,
    BLK_HEADER_PREV_FREE_BIT = 1 << 1,

    BLK_HEADER_OVERHEAD = sizeof(size_t),
    BLK_START_OFFSET = offsetof(rtl_block_header_t, size) + sizeof(size_t),

    //
    // A free block must be large enough to store its header minus the size of
    // the prev_phys_block field, and no larger than the number of addressable
    // bits for FL_INDEX.
    //
    BLK_SIZE_MIN = sizeof(rtl_block_header_t) - sizeof(rtl_block_header_t*),
    BLK_SIZE_MAX = ((size_t)1) << FL_INDEX_MAX,
    
    //
    // Overhead of the TLSF structures in a given memory block passed to
    // fx_mem_pool_add, equal to the overhead of a free block and the
    // sentinel block.
    //
    POOL_OVERHEAD = 2 * BLK_HEADER_OVERHEAD,
};

static inline int 
tlsf_ffs(unsigned int word)
{
    const unsigned int reverse = word & (~word + 1);
    const int bit = 32 - hw_cpu_clz(reverse);
    return bit - 1;
}

static inline int 
tlsf_fls(unsigned int word)
{
    const int bit = word ? 32 - hw_cpu_clz(word) : 0;
    return bit - 1;
}

STATIC inline size_t 
block_size(const rtl_block_header_t* block)
{
    return block->size & ~(BLK_HEADER_FREE_BIT | BLK_HEADER_PREV_FREE_BIT);
}

static inline void 
block_set_size(rtl_block_header_t* block, size_t size)
{
    const size_t oldsize = block->size;
    block->size = size | 
        (oldsize & (BLK_HEADER_FREE_BIT | BLK_HEADER_PREV_FREE_BIT));
}

#ifdef FX_DBG_ENABLED
STATIC inline int 
block_is_last(const rtl_block_header_t* block)
{
    return block_size(block) == 0;
}
#endif

STATIC inline int 
block_is_free(const rtl_block_header_t* block)
{
    return (int) (block->size & BLK_HEADER_FREE_BIT);
}

static inline void 
block_set_free(rtl_block_header_t* block)
{
    block->size |= BLK_HEADER_FREE_BIT;
}

static inline void 
block_set_used(rtl_block_header_t* block)
{
    block->size &= ~BLK_HEADER_FREE_BIT;
}

STATIC inline int 
block_is_prev_free(const rtl_block_header_t* block)
{
    return (int) (block->size & BLK_HEADER_PREV_FREE_BIT);
}

static inline void 
block_set_prev_free(rtl_block_header_t* block)
{
    block->size |= BLK_HEADER_PREV_FREE_BIT;
}

static inline void 
block_set_prev_used(rtl_block_header_t* block)
{
    block->size &= ~BLK_HEADER_PREV_FREE_BIT;
}

STATIC inline rtl_block_header_t*
block_from_ptr(const void *ptr)
{
    return (rtl_block_header_t*) (((unsigned char*) ptr) - BLK_START_OFFSET);
}

STATIC inline void*
block_to_ptr(const rtl_block_header_t* block)
{
    return (void*) (((unsigned char*) block) + BLK_START_OFFSET);
}

STATIC inline rtl_block_header_t*
offset_to_block(const void* ptr, ptrdiff_t size)
{
    return (rtl_block_header_t*) (((ptrdiff_t) ptr) + size);
}

STATIC inline rtl_block_header_t*
block_prev(const rtl_block_header_t* block)
{
    fx_dbg_assert(block_is_prev_free(block));
    return block->prev_phys_block;
}

STATIC inline rtl_block_header_t*
block_next(const rtl_block_header_t* block)
{
    rtl_block_header_t* next = offset_to_block(
        block_to_ptr(block), 
        block_size(block)-BLK_HEADER_OVERHEAD
    );

    fx_dbg_assert(!block_is_last(block));
    return next;
}

static inline rtl_block_header_t* 
block_link_next(rtl_block_header_t* block)
{
    rtl_block_header_t *next = block_next(block);
    next->prev_phys_block = block;
    return next;
}

static inline void 
block_mark_as_free(rtl_block_header_t* block)
{
    //
    // Link the block to the next block, first.
    //
    rtl_block_header_t *next = block_link_next(block);
    block_set_prev_free(next);
    block_set_free(block);
}

static inline void 
block_mark_as_used(rtl_block_header_t* block)
{
    rtl_block_header_t *next = block_next(block);
    block_set_prev_used(next);
    block_set_used(block);
}

static inline size_t 
align_up(size_t x, size_t align)
{
    fx_dbg_assert(0 == (align & (align - 1)));
    return (x + (align - 1)) & ~(align - 1);
}

static inline size_t 
align_down(size_t x, size_t align)
{
    fx_dbg_assert(0 == (align & (align - 1)));
    return x - (x & (align - 1));
}

#ifdef FX_DBG_ENABLED
static inline void*
align_ptr(const void *ptr, size_t align)
{
    const ptrdiff_t aligned = (((ptrdiff_t) ptr) + (align - 1)) & ~(align - 1);
    fx_dbg_assert(0 == (align & (align - 1)));
    return (void*) aligned;
}
#endif

static inline size_t 
adjust_request_size(size_t size, size_t align)
{
    size_t adjust = 0;

    if (size) 
    {
        const size_t aligned = align_up(size, align);

        /* aligned sized must not exceed BLK_SIZE_MAX */
        if (aligned < BLK_SIZE_MAX) 
        {
            adjust = lang_max(aligned, BLK_SIZE_MIN);
        }
    }

    return adjust;
}

//
// TLSF utility functions. In most cases, these are direct translations of
// the documentation found in the white paper.
//

STATIC void 
mapping_insert(size_t size, int *fli, int *sli)
{
    int fl, sl;

    if (size < SMALL_BLOCK_SIZE) 
    {
        // Store small blocks in first list.

        fl = 0;
        sl = ((int) size) / (SMALL_BLOCK_SIZE / SL_INDEX_COUNT);
    } 
    else 
    {
        fl = tlsf_fls(size);
        sl = (int) (
            (size >> (fl - SL_INDEX_COUNT_LOG2)) ^ (1 << SL_INDEX_COUNT_LOG2)
        );
        fl -= (FL_INDEX_SHIFT - 1);
    }

    *fli = fl;
    *sli = sl;
}

static inline void
mapping_search(size_t size, int *fli, int *sli)
{
    if (size >= SMALL_BLOCK_SIZE) 
    {
        const size_t round = (1 << (tlsf_fls(size) - SL_INDEX_COUNT_LOG2)) - 1;
        size += round;
    }

    mapping_insert(size, fli, sli);
}

static rtl_block_header_t* 
search_suitable_block(rtl_mem_pool_t* control, int* fli, int* sli)
{
    int fl = *fli;
    int sl = *sli;

    //
    // First, search for a block in the list associated with the given fl/sl 
    // index.
    //
    unsigned int sl_map = control->sl_bitmap[fl] & (((unsigned int)~0) << sl);

    if (!sl_map) 
    {
        //
        // No block exists. Search in the next largest first-level list.
        //
        const unsigned int fl_map = 
            control->fl_bitmap & (((unsigned int)~0) << (fl + 1));
        
        if (!fl_map) 
        {
            return NULL;
        }

        fl = tlsf_ffs(fl_map);
        *fli = fl;
        sl_map = control->sl_bitmap[fl];
    }

    fx_dbg_assert(sl_map);
    sl = tlsf_ffs(sl_map);
    *sli = sl;

    //
    // Return the first block in the free list.
    //
    return control->blocks[fl][sl];
}

//
// Remove a free block from the free list.
//
static void 
remove_free_block(rtl_mem_pool_t* control, rtl_block_header_t* block, int fl, int sl)
{
    rtl_block_header_t *prev = block->prev_free;
    rtl_block_header_t *next = block->next_free;
    fx_dbg_assert(prev);
    fx_dbg_assert(next);
    next->prev_free = prev;
    prev->next_free = next;

    // If this block is the head of the free list, set new head.
    if (control->blocks[fl][sl] == block) 
    {
        control->blocks[fl][sl] = next;

        // If the new head is null, clear the bitmap.
        if (next == &control->block_null) 
        {
            control->sl_bitmap[fl] &= ~(1 << sl);

            // If the second bitmap is now empty, clear the fl bitmap.
            if (!control->sl_bitmap[fl]) 
            {
                control->fl_bitmap &= ~(1 << fl);
            }
        }
    }
}

static inline void 
insert_free_block(rtl_mem_pool_t* control, rtl_block_header_t* block, int fl, int sl)
{
    rtl_block_header_t *current = control->blocks[fl][sl];
    fx_dbg_assert(current);
    fx_dbg_assert(block);
    block->next_free = current;
    block->prev_free = &control->block_null;
    current->prev_free = block;

    fx_dbg_assert(
        block_to_ptr(block) == align_ptr(block_to_ptr(block), ALIGN_SIZE)
    );
    
    //
    // Insert the new block at the head of the list, and mark the first-
    // and second-level bitmaps appropriately.
    //
    control->blocks[fl][sl] = block;
    control->fl_bitmap |= (1 << fl);
    control->sl_bitmap[fl] |= (1 << sl);
}

static inline void 
block_remove(rtl_mem_pool_t *control, rtl_block_header_t* block)
{
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);
    remove_free_block(control, block, fl, sl);
}

static inline void 
block_insert(rtl_mem_pool_t *control, rtl_block_header_t* block)
{
    int fl, sl;
    mapping_insert(block_size(block), &fl, &sl);
    insert_free_block(control, block, fl, sl);
}

static inline int 
block_can_split(rtl_block_header_t* block, size_t size)
{
    return block_size(block) >= sizeof(rtl_block_header_t) + size;
}

static inline rtl_block_header_t*
block_split(rtl_block_header_t* block, size_t size)
{
    //
    // Calculate the amount of space left in the remaining block.
    //
    rtl_block_header_t *remaining = offset_to_block(
        block_to_ptr(block), size - BLK_HEADER_OVERHEAD
    );

    const size_t remain_size = block_size(block) - (size + BLK_HEADER_OVERHEAD);

    fx_dbg_assert(
        block_to_ptr(remaining) == align_ptr(block_to_ptr(remaining),ALIGN_SIZE)
    );

    fx_dbg_assert(block_size(block) == remain_size + size +BLK_HEADER_OVERHEAD);
    block_set_size(remaining, remain_size);
    fx_dbg_assert(block_size(remaining) >= BLK_SIZE_MIN);

    block_set_size(block, size);
    block_mark_as_free(remaining);
    return remaining;
}

static inline rtl_block_header_t* 
block_absorb(rtl_block_header_t* prev, rtl_block_header_t* block)
{
    fx_dbg_assert(!block_is_last(prev));

    // Note: Leaves flags untouched.
    prev->size += block_size(block) + BLK_HEADER_OVERHEAD;
    block_link_next(prev);
    return prev;
}

//
// Merge a just-freed block with an adjacent previous free block.
//
static inline rtl_block_header_t*
block_merge_prev(rtl_mem_pool_t* control, rtl_block_header_t* block)
{
    if (block_is_prev_free(block)) 
    {
        rtl_block_header_t *prev = block_prev(block);
        fx_dbg_assert(prev);
        fx_dbg_assert(block_is_free(prev));
        block_remove(control, prev);
        block = block_absorb(prev, block);
    }

    return block;
}

//
// Merge a just-freed block with an adjacent free block.
//
static inline rtl_block_header_t*
block_merge_next(rtl_mem_pool_t* control, rtl_block_header_t* block)
{
    rtl_block_header_t *next = block_next(block);
    fx_dbg_assert(next);

    if (block_is_free(next)) 
    {
        fx_dbg_assert(!block_is_last(block));
        block_remove(control, next);
        block = block_absorb(block, next);
    }

    return block;
}

//
// Trim any trailing block space off the end of a block, return to pool.
//
static void 
block_trim_free(rtl_mem_pool_t* control, rtl_block_header_t* block, size_t size)
{
    fx_dbg_assert(block_is_free(block));

    if (block_can_split(block, size)) 
    {
        rtl_block_header_t *remaining_block = block_split(block, size);
        block_link_next(block);
        block_set_prev_free(remaining_block);
        block_insert(control, remaining_block);
    }
}

static rtl_block_header_t*
block_locate_free(rtl_mem_pool_t* control, size_t size)
{
    int fl = 0, sl = 0;
    rtl_block_header_t* block = NULL;

    if (size) 
    {
        mapping_search(size, &fl, &sl);
        block = search_suitable_block(control, &fl, &sl);
    }

    if (block) 
    {
        fx_dbg_assert(block_size(block) >= size);
        remove_free_block(control, block, fl, sl);
    }

    if (block && !block->size)
    {
        block = NULL;
    }

    return block;
}

static void*
block_prepare_used(rtl_mem_pool_t* control, rtl_block_header_t* block, size_t size)
{
    void *p = NULL;

    if (block) 
    {
        fx_dbg_assert(size);
        block_trim_free(control, block, size);
        block_mark_as_used(block);
        p = block_to_ptr(block);
    }
    return p;
}

//!
//! Memory pool initialization. After initialization pool has no memory.
//! @param pool Memory pool to be initialized.
//!
void 
rtl_mem_pool_init(rtl_mem_pool_t* pool)
{
    int i, j;

    pool->block_null.next_free = &pool->block_null;
    pool->block_null.prev_free = &pool->block_null;

    pool->fl_bitmap = 0;
    for (i = 0; i < FL_INDEX_COUNT; ++i) 
    {
        pool->sl_bitmap[i] = 0;
        for (j = 0; j < SL_INDEX_COUNT; ++j) 
        {
            pool->blocks[i][j] = &pool->block_null;
        }
    }
}

//!
//! Add memory to memory pool.
//! This function splits specified region to set of memory blocks and add all 
//! of these blocks into the memory pool.
//! @param pool Initialized memory pool.
//! @param base Base addr of contiguous memory chunk to be added into the pool.
//! @param size Size of memory chunk to be added into the pool.
//! @return true in case of success, false otherwise.
//!
bool 
rtl_mem_pool_add_mem(rtl_mem_pool_t* pool, void* mem, size_t bytes)
{
    rtl_block_header_t* block;
    rtl_block_header_t *next;
    const size_t pool_overhead = POOL_OVERHEAD;
    const size_t pool_bytes = align_down(bytes - pool_overhead, ALIGN_SIZE); 

    if (pool_bytes < BLK_SIZE_MIN || pool_bytes > BLK_SIZE_MAX) 
    {
        return false;
    }

    //
    // Create the main free block. Offset the start of the block slightly
    // so that the prev_phys_block field falls outside of the pool -
    // it will never be used.
    //
    block = offset_to_block(mem, -(ptrdiff_t)BLK_HEADER_OVERHEAD);
    block_set_size(block, pool_bytes);
    block_set_free(block);
    block_set_prev_used(block);

    block_insert(pool, block);

    //
    // Split the block to create a zero-size sentinel block.
    //
    next = block_link_next(block);
    block_set_size(next, 0);
    block_set_used(next);
    block_set_prev_free(next);

    return true;
}

//!
//! Allocates memory from specified pool.
//! Before allocation, pool must be properly initialized and memory must be 
//! added into the pool.
//! @param pool Initialized memory pool.
//! @param alloc_size Size of memory to be allocated from the pool.
//! @param ptr Pointer to pointer to allocated memory.
//! @return pointer to allocated memory or NULL.
//!
void*
rtl_mem_pool_alloc(rtl_mem_pool_t* pool, size_t size)
{
    const size_t adjust = adjust_request_size(size, ALIGN_SIZE);
    rtl_block_header_t* block = block_locate_free(pool, adjust);
    void* p = block_prepare_used(pool, block, adjust);
    return p;
}

//!
//! Returns memory to specified pool.
//! As a part of freeing, memory defragmentation will be performed.
//! @param pool Initialized memory pool.
//! @param ptr Pointer to block, allocated with @ref rtl_mem_pool_alloc.
//!
void 
rtl_mem_pool_free(rtl_mem_pool_t* pool, void* ptr)
{
    rtl_block_header_t* block = block_from_ptr(ptr);
    fx_dbg_assert(!block_is_free(block));
    block_mark_as_free(block);
    block = block_merge_prev(pool, block);
    block = block_merge_next(pool, block);
    block_insert(pool, block);
}

static inline size_t
get_max_chunk_above_small_sz(int fl, int sl)
{
    const unsigned int sub_div = 
        1 << (fl + FL_INDEX_SHIFT - SL_INDEX_COUNT_LOG2 - 1);
    return (1 << (fl + FL_INDEX_SHIFT - 1)) + sub_div * sl;
}

static inline size_t
get_max_chunk_below_small_sz(int sl)
{
    return sl * (SMALL_BLOCK_SIZE / SL_INDEX_COUNT);
}

size_t 
rtl_mem_pool_get_max_blk(rtl_mem_pool_t* pool)
{
    size_t blk_sz = 0;
    int fl = tlsf_fls(pool->fl_bitmap);   

    if (fl >= 0)
    {
        const int sl = tlsf_fls(pool->sl_bitmap[fl]);
        blk_sz = fl ? 
            get_max_chunk_above_small_sz(fl, sl) : 
            get_max_chunk_below_small_sz(sl);
    }

    return blk_sz;
}
