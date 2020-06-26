#ifndef _FX_BLOCK_POOL_V1_HEADER_
#define _FX_BLOCK_POOL_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_block_pool.h
  *  @brief  Interface of memory block pool primitive.
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2008-2020.
  *  Redistribution and use in source and binary forms, with or without 
  *  modification, are permitted provided that the following conditions are met:
  *  1. Redistributions of source code must retain the above copyright notice,
  *     this list of conditions and the following disclaimer.
  *  2. Redistributions in binary form must reproduce the above copyright 
  *     notice, this list of conditions and the following disclaimer in the 
  *     documentation and/or other materials provided with the distribution.
  *  3. Neither the name of the copyright holder nor the names of its 
  *     contributors may be used to endorse or promote products derived from 
  *     this software without specific prior written permission.
  *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
  *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
  *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
  *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
  *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
  *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
  *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
  *  POSSIBILITY OF SUCH DAMAGE.
  *****************************************************************************/

#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)

enum
{
    FX_BLOCK_POOL_MAGIC = 0x424C4B50, // 'BLKP'
    FX_BLOCK_POOL_OK = FX_STATUS_OK,
    FX_BLOCK_POOL_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_BLOCK_POOL_INVALID_OBJ,
    FX_BLOCK_POOL_NO_MEM,
    FX_BLOCK_POOL_IMPROPER_ALIGN,
    FX_BLOCK_POOL_UNSUPPORTED_POLICY,
    FX_BLOCK_POOL_ERR_MAX
};

//!
//! Block pool representation. 
//!
typedef struct
{
    fx_sync_waitable_t waitable;  //!< Internal waitable object.
    fx_rtp_t rtp;                 //!< Runtime protection member (canary).
    lock_t  lock;                 //!< Lock associated with the primitive.
    uintptr_t base;               //!< Address of available memory blocks pool.
    size_t sz;                    //!< Size of memory block.
    size_t remaining_sz;          //!< Remaining memory size in pool.
    rtl_list_t free_blocks;       //!< List of bree blocks.
    unsigned int free_blocks_num; //!< Available blocks count.
    fx_sync_policy_t policy;      //!< Default releasing policy.
} 
fx_block_pool_t;

//!
//! Block header. 
//!
typedef struct
{
    union
    {
        fx_block_pool_t* parent_pool;
        rtl_list_linkage_t link;
    }
    hdr;
} 
fx_mem_block_t;

int fx_block_pool_init(
    fx_block_pool_t* bp, 
    void* base, 
    size_t sz, 
    size_t blk_sz, 
    fx_sync_policy_t p
);
int fx_block_pool_deinit(fx_block_pool_t* bp);
int fx_block_pool_alloc(fx_block_pool_t* bp, void** blk, fx_event_t* cancel);
int fx_block_pool_timedalloc(fx_block_pool_t* bp, void** blk, uint32_t tout);
int fx_block_pool_release(void* blk_ptr);
int fx_block_pool_release_internal(void* blk_ptr, fx_sync_policy_t p);
int fx_block_pool_avail_blocks(fx_block_pool_t* bp, unsigned int* count);

FX_METADATA(({ interface: [FX_BLOCK_POOL, V1] })) 

#endif
