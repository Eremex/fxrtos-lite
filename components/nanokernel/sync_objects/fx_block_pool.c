/** 
  ******************************************************************************
  *  @file   fx_block_pool.c
  *  @brief  Implementation of memory block pool primitive.
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

#include FX_INTERFACE(FX_BLOCK_POOL)

FX_METADATA(({ implementation: [FX_BLOCK_POOL, V1] }))

#define fx_block_pool_is_valid(bp) \
    (fx_rtp_check(&((bp)->rtp), FX_BLOCK_POOL_MAGIC))

//!
//! Test and wait function. It is used for wait implementation.
//! @param [in] object Pool object to be tested.
//! @param [in] wb Wait block to be inserted into queue. 
//! @param [in] wait Wait option used to test object. 
//! @return true in case of object is signaled, false otherwise. If object is 
//! nonsignaled it means that wait block has been inserted into the queue.
//! @remark SPL = SCHED_LEVEL
//!
static bool 
fx_block_pool_test(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait)
{
    bool wait_satisfied = false;
    fx_mem_block_t* block = NULL;
    fx_block_pool_t* const bp = lang_containing_record(
        object, 
        fx_block_pool_t, 
        waitable
    );

    fx_sync_waitable_lock(object);

    if (!rtl_list_empty(&(bp->free_blocks)))
    { 
        block = rtl_list_entry(
            rtl_list_first(&bp->free_blocks), 
            fx_mem_block_t, 
            hdr.link
        );

        rtl_list_remove(&(block->hdr.link));    
    }
    else if (bp->remaining_sz >= bp->sz)
    {
        block = (fx_mem_block_t*)bp->base;
        bp->base += bp->sz;
        bp->remaining_sz -= bp->sz;
    }
    else if (wait)
    {
        _fx_sync_wait_start(object, wb);
    }
    
    if (block)
    {
        //
        // Get location where address of allocated block should be stored.
        // This variable is specified by user at "alloc" calls.
        //
        void** usr_storage = (void**)fx_sync_wait_block_get_attr(wb);

        //
        // Save pointer to parent pool into the block header, and set allocated 
        // memory pointerjust behind parent pool pointer.
        // N.B. List linkage in the header is used only when the block is free, 
        // so, no need to preserve space for list linkage.
        //
        block->hdr.parent_pool = bp;
        *(usr_storage) = (((char*)block) + sizeof(block->hdr.parent_pool));
        --(bp->free_blocks_num);
        wait_satisfied = true;
    }

    fx_sync_waitable_unlock(object); 
    return wait_satisfied; 
}

//!
//! Initializes a memory block pool.
//! @param [in,out] bp Block pool object to be initialized.
//! @param [in] base_ptr Pointer to user-allocated memory where the pool should 
//! reside (should be aligned to at least sizeof(unitptr_t).
//! @param [in] sz Size of user-allocd memory (should be aligned to uintptr_t).
//! @param [in] blk_sz Size of one memory block.
//! @param [in] p Default policy of waiters releasing.
//! @return FX_BLOCK_POOL_OK in case of successful initialization, error code 
//! otherwise.
//!
int 
fx_block_pool_init(
    fx_block_pool_t* bp, 
    void* base_ptr, 
    size_t sz, 
    size_t blk_sz, 
    fx_sync_policy_t p)
{
    const size_t ptr_sz = sizeof(uintptr_t);
    const size_t round_blk_sz = ((blk_sz + ptr_sz - 1) / ptr_sz) * ptr_sz;
    const unsigned int block_full_sz = ptr_sz + round_blk_sz;
    const unsigned int blk_num = sz / block_full_sz;

    lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(base_ptr != NULL, FX_BLOCK_POOL_NO_MEM);
    lang_param_assert(blk_sz > 0, FX_BLOCK_POOL_NO_MEM);
    lang_param_assert(p < FX_SYNC_POLICY_MAX, FX_BLOCK_POOL_UNSUPPORTED_POLICY);
    lang_param_assert(
        (((uintptr_t)base_ptr) & (sizeof(uintptr_t)-1)) == 0, 
        FX_BLOCK_POOL_IMPROPER_ALIGN
    );
    lang_param_assert(sz >= block_full_sz, FX_BLOCK_POOL_NO_MEM);
    lang_param_assert(
        sizeof(fx_mem_block_t) <= block_full_sz, 
        FX_BLOCK_POOL_NO_MEM
    );
        
    fx_rtp_init(&bp->rtp, FX_BLOCK_POOL_MAGIC);
    fx_spl_spinlock_init(&bp->lock);
    fx_sync_waitable_init(&bp->waitable, &bp->lock, fx_block_pool_test);
    rtl_list_init(&bp->free_blocks);
    bp->free_blocks_num = blk_num;
    bp->sz = block_full_sz;
    bp->base = (uintptr_t)base_ptr;
    bp->remaining_sz = sz;

    return FX_BLOCK_POOL_OK;
}

//!
//! Destructor of memory block pool.
//! @param [in,out] cond Pool object to be deinitialized.
//! @return FX_BLOCK_POOL_OK in case of successful destruction, error code 
//! otherwise.
//!
int 
fx_block_pool_deinit(fx_block_pool_t* bp)
{
    fx_sched_state_t prev;
    lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(fx_block_pool_is_valid(bp), FX_BLOCK_POOL_INVALID_OBJ);  

    fx_sched_lock(&prev);
    fx_rtp_deinit(&bp->rtp);
    fx_sync_waitable_lock(&bp->waitable);
    _fx_sync_wait_notify(&bp->waitable, FX_WAIT_DELETED, NULL);
    fx_sync_waitable_unlock(&bp->waitable);
    fx_sched_unlock(prev);
    return FX_BLOCK_POOL_OK;
}

//!
//! Allocates memory block from pool.
//! If no free block available, this function suspends until some block will be 
//! freed or cancel event is set.
//! @param [in] bp Pool object to allocate from.
//! @param [out] allocated_blk Pointer to allocated block if succeeded,
//! unchanged if function fails.
//! @param [in] cancel_event Optional cancel event (can be NULL).
//! @return Wait status.
//!
int 
fx_block_pool_alloc(
    fx_block_pool_t* bp, 
    void** allocated_blk, 
    fx_event_t* cancel_event)
{
    void* ptr;
    int res = FX_STATUS_OK;
    lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(fx_block_pool_is_valid(bp), FX_BLOCK_POOL_INVALID_OBJ);  

    res = fx_thread_wait_object(&bp->waitable, &ptr, cancel_event);

    if (res == FX_THREAD_OK)
    {
        *allocated_blk = ptr;
    }
    return res;
}

//!
//! Allocates memory block from pool.
//! If no free block available, this function suspends until some block will be 
//! freed or timeout  exceeded.
//! @param [in] bp Pool object to allocate from.
//! @param [out] allocated_blk Pointer to allocated block if succeeded, 
//! unchanged if function fails.
//! @param [in] tout Timeout value (in ticks) or FX_THREAD_INFINITE_TIMEOUT.
//! @return Wait status.
//!
int 
fx_block_pool_timedalloc(
    fx_block_pool_t* bp, 
    void** allocated_blk, 
    uint32_t tout)
{
    void* ptr;
    int res = FX_BLOCK_POOL_OK;
    lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(fx_block_pool_is_valid(bp), FX_BLOCK_POOL_INVALID_OBJ);  

    res = fx_thread_timedwait_object(&bp->waitable, &ptr, tout);

    if (res == FX_THREAD_OK)
    {
        *allocated_blk = ptr;
    }
    return res;  
}

//!
//! Returns previously allocated memory block into pool.
//! If some thread is waiting for pool (when block will be available) it will 
//! be released with specified notification policy.
//! @param [in] block_ptr Pointer to memory block returned by 
//! @ref fx_block_pool_timedalloc or @ref fx_block_pool_alloc.
//! @param [in] p Waiter releasing policy.
//! @return FX_BLOCK_POOL_OK if succeeded, error code othrewise.
//!
int 
fx_block_pool_release_internal(void* block_ptr, const fx_sync_policy_t p)
{
    int res = FX_BLOCK_POOL_OK;
    lang_param_assert(block_ptr != NULL, FX_BLOCK_POOL_INVALID_PTR);
    {
        fx_mem_block_t* const blk = (fx_mem_block_t*)(
            ((char*)block_ptr) - sizeof(uintptr_t)
        );
        
        fx_block_pool_t* const bp = blk->hdr.parent_pool;
        fx_sched_state_t prev;

        lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
        lang_param_assert(fx_block_pool_is_valid(bp),FX_BLOCK_POOL_INVALID_OBJ);
            
        fx_sched_lock(&prev);
        fx_sync_waitable_lock(&bp->waitable);

        //
        // If the objects has waiters pended on it, get pointer of variable 
        // where address of the block should be stored, save pointer to our 
        // block directly into buffer of waiting thread, and release the waiter.
        // Otherwise just return block into pool of free blocks.
        //
        if (_fx_sync_waitable_nonempty(&bp->waitable)) 
        {
            fx_sync_wait_block_t* wb =_fx_sync_wait_block_get(&bp->waitable, p);
            void** usr_storage = (void**)fx_sync_wait_block_get_attr(wb);
            *(usr_storage) = block_ptr;
            _fx_sync_wait_notify(&bp->waitable, FX_WAIT_SATISFIED, wb);
        }
        else 
        {
            rtl_list_insert(&bp->free_blocks, &blk->hdr.link);
            ++(bp->free_blocks_num);
        }
        fx_sync_waitable_unlock(&bp->waitable);
        fx_sched_unlock(prev);
    }
    return res;
}

//!
//! Returns previously allocated memory block into pool.
//! If some thread is waiting for pool (when block will be available) it will 
//! be released with default notification policy.
//! @param [in] block_ptr Pointer to memory block returned by either
//! @ref fx_block_pool_timedalloc or @ref fx_block_pool_alloc.
//! @return FX_STATUS_OK if succeeded, error code othrewise.
//!
int 
fx_block_pool_release(void* block_ptr)
{
    int res = FX_BLOCK_POOL_OK;
    lang_param_assert(block_ptr != NULL, FX_BLOCK_POOL_INVALID_PTR);
    {
        fx_mem_block_t* const blk = (fx_mem_block_t*)(
            ((char*)block_ptr) - sizeof(uintptr_t)
        );
        fx_block_pool_t* const bp = blk->hdr.parent_pool;

        lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
        lang_param_assert(fx_block_pool_is_valid(bp),FX_BLOCK_POOL_INVALID_OBJ); 

        res = fx_block_pool_release_internal(block_ptr, bp->policy);
    }
    return res;
}

//!
//! Returns number of free blocks in the block pool.
//! @param [in] bp Block pool to be analyzed.
//! @param [in] count Pointer to variable where number of free block should be 
//! stored.
//! @return FX_STATUS_OK if succeeded, error code othrewise.
//!
int 
fx_block_pool_avail_blocks(fx_block_pool_t* bp, unsigned int* count)
{
    lang_param_assert(bp != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(count != NULL, FX_BLOCK_POOL_INVALID_PTR);
    lang_param_assert(fx_block_pool_is_valid(bp), FX_BLOCK_POOL_INVALID_OBJ); 
    
    *count = bp->free_blocks_num;
    return FX_STATUS_OK;
}
