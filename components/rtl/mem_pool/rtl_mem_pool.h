#ifndef _RTL_MEM_POOL_TLSF_HEADER_
#define _RTL_MEM_POOL_TLSF_HEADER_

/*
 * Two Level Segregated Fit memory allocator.
 *
 * Copyright (c) 2018 Eremex Ltd.
 * Copyright (c) 2016 National Cheng Kung University, Taiwan.
 * Copyright (c) 2006-2008, 2011, 2014 Matthew Conte.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the copyright holder nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL MATTHEW CONTE BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include FX_INTERFACE(CFG_OPTIONS)

#ifndef RTL_MEM_POOL_SUBDIV_LOG2
#define RTL_MEM_POOL_SUBDIV_LOG2 4
#endif

#ifndef RTL_MEM_POOL_MAX_CHUNK
#error RTL_MEM_POOL_MAX_CHUNK is not defined!
#endif

//
// Configuration constants.
//
enum 
{
    FL_INDEX_MAX = RTL_MEM_POOL_MAX_CHUNK,

    // log2 of number of linear subdivisions of block sizes. Larger
    // values require more memory in the control structure. Values of
    // 4 or 5 are typical.
    //
    SL_INDEX_COUNT_LOG2 = RTL_MEM_POOL_SUBDIV_LOG2,

    //
    // All allocation sizes and addresses are aligned.
    //
    ALIGN_SIZE_LOG2 = sizeof(void*) == 8 ? 3 : 2,
    ALIGN_SIZE = (1 << ALIGN_SIZE_LOG2),

    //
    // We support allocations of sizes up to (1 << FL_INDEX_MAX) bits.
    // However, because we linearly subdivide the second-level lists, and
    // our minimum size granularity is 4 bytes, it doesn't make sense to
    // create first-level lists for sizes smaller than SL_INDEX_COUNT * 4,
    // or (1 << (SL_INDEX_COUNT_LOG2 + 2)) bytes, as there we will be
    // trying to split size ranges into more slots than we have available.
    // Instead, we calculate the minimum threshold size, and place all
    // blocks below that size into the 0th first-level list.
    //
    SL_INDEX_COUNT = (1 << SL_INDEX_COUNT_LOG2),
    FL_INDEX_SHIFT = (SL_INDEX_COUNT_LOG2 + ALIGN_SIZE_LOG2),
    FL_INDEX_COUNT = (FL_INDEX_MAX - FL_INDEX_SHIFT + 1),

    SMALL_BLOCK_SIZE = (1 << FL_INDEX_SHIFT),
};

//
// Block header structure.
//
// There are several implementation subtleties involved:
// - The prev_phys_block field is only valid if the previous block is free.
// - The prev_phys_block field is actually stored at the end of the
//   previous block. It appears at the beginning of this structure only to
//   simplify the implementation.
// - The next_free / prev_free fields are only valid if the block is free.
//
typedef struct _rtl_block_header_t 
{
    struct _rtl_block_header_t* prev_phys_block;
    size_t size;            //!< The size of this block, minus the block header.
    struct _rtl_block_header_t *next_free;
    struct _rtl_block_header_t *prev_free;
} 
rtl_block_header_t;

//
// The TLSF pool structure.
//
typedef struct 
{
    rtl_block_header_t block_null;
    unsigned int fl_bitmap;
    unsigned int sl_bitmap[FL_INDEX_COUNT];
    rtl_block_header_t *blocks[FL_INDEX_COUNT][SL_INDEX_COUNT];
} 
rtl_mem_pool_t;

void rtl_mem_pool_init(rtl_mem_pool_t* pool);
bool rtl_mem_pool_add_mem(rtl_mem_pool_t* pool, void* mem, size_t bytes);
void* rtl_mem_pool_alloc(rtl_mem_pool_t* pool, size_t bytes);
void rtl_mem_pool_free(rtl_mem_pool_t* pool, void* ptr);
size_t rtl_mem_pool_get_max_blk(rtl_mem_pool_t* pool);

FX_METADATA(({ interface: [RTL_MEM_POOL, TLSF] }))

#endif
