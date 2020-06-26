/** 
  ******************************************************************************
  *  @file   fx_mem_pool.c
  *  @brief  Wrapper for memory pool.
  *
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

#include FX_INTERFACE(FX_DBG)
#include FX_INTERFACE(FX_SCHED)
#include FX_INTERFACE(HW_CPU)
#include FX_INTERFACE(FX_MEM_POOL)

FX_METADATA(({ implementation: [FX_MEM_POOL, TLSF] }))

//!
//! Memory pool initialization. After initialization pool has no memory.
//! @param pool Memory pool to be initialized.
//! @return FX_MEM_POOL_OK in case of success, error code otherwise.
//! @sa fx_mem_pool_alloc
//! @sa fx_mem_pool_free
//! @sa fx_mem_pool_add_mem
//!
int 
fx_mem_pool_init(fx_mem_pool_t* pool)
{
    lang_param_assert(pool != NULL, FX_MEM_POOL_INVALID_PTR);

    rtl_mem_pool_init(&pool->rtl_pool);
    fx_spl_spinlock_init(&pool->lock);
    return FX_MEM_POOL_OK;
}

//!
//! Memory pool deinitialization.
//! @param pool Memory pool to be deinitialized.
//! @return FX_MEM_POOL_OK in case of success, error code otherwise.
//!
int 
fx_mem_pool_deinit(fx_mem_pool_t* pool)
{
    return FX_MEM_POOL_OK;
}

//!
//! Adds memory to memory pool.
//! This function splits specified region to set of memory blocks and add all of
//! these blocks into the memory pool.
//! @param pool Initialized memory pool.
//! @param base Base addr of contiguous memory chunk to be added into the pool.
//! @param size Size of memory chunk to be added into the pool.
//! @return FX_MEM_POOL_OK in case of success, error code otherwise.
//! @remark SPL <= SCHED_LEVEL
//! @sa fx_mem_pool_alloc
//! @sa fx_mem_pool_free
//! @sa fx_mem_pool_init
//!
int 
fx_mem_pool_add_mem(fx_mem_pool_t* pool, uintptr_t mem, size_t bytes)
{
    bool success = false;
    fx_sched_state_t state;
    lang_param_assert(pool != NULL, FX_MEM_POOL_INVALID_PTR);
    
    if (((ptrdiff_t)mem % ALIGN_SIZE) != 0) 
    {
        return FX_MEM_POOL_INVALID_BUF;
    }

    fx_sched_lock(&state);
    fx_spl_spinlock_get_from_sched(&pool->lock);
    success = rtl_mem_pool_add_mem(&pool->rtl_pool, (void*) mem, bytes);
    fx_spl_spinlock_put_from_sched(&pool->lock);
    fx_sched_unlock(state);
    return success ? FX_MEM_POOL_OK : FX_MEM_POOL_INVALID_PTR;   
}

//!
//! Allocates memory from specified pool.
//! Before allocation, pool must be properly initialized and memory must be 
//! added into the pool.
//! @param pool Initialized memory pool.
//! @param alloc_size Size of memory to be allocated from the pool.
//! @param ptr Pointer to pointer to allocated memory.
//! @return FX_MEM_POOL_OK in case of success, error code otherwise.
//! @sa fx_mem_pool_init
//! @sa fx_mem_pool_free
//! @sa fx_mem_pool_add_mem
//!
int
fx_mem_pool_alloc(fx_mem_pool_t* pool, size_t size, void** p)
{
    void* ptr = NULL;
    fx_sched_state_t state;
    lang_param_assert(pool != NULL, FX_MEM_POOL_INVALID_PTR);
    lang_param_assert(size > 0, FX_MEM_POOL_ZERO_SZ);
    lang_param_assert(p != NULL, FX_MEM_POOL_INVALID_PTR);

    fx_sched_lock(&state);
    fx_spl_spinlock_get_from_sched(&pool->lock);
    ptr = rtl_mem_pool_alloc(&pool->rtl_pool, size);
    *p = ptr;
    fx_spl_spinlock_put_from_sched(&pool->lock);
    fx_sched_unlock(state);
    return ptr ? FX_MEM_POOL_OK : FX_MEM_POOL_NO_MEM;
}

//!
//! Returns memory to specified pool.
//! As a part of freeing, memory defragmentation will be performed.
//! @param pool Initialized memory pool.
//! @param ptr Pointer to block, allocated with @ref fx_mem_pool_alloc.
//! @return FX_STATUS_OK in case of success, error code otherwise.
//! @sa fx_mem_pool_init
//! @sa fx_mem_pool_alloc
//! @sa fx_mem_pool_add_mem
//!
int 
fx_mem_pool_free(fx_mem_pool_t* pool, void* ptr)
{
    fx_sched_state_t state;
    lang_param_assert(pool != NULL, FX_MEM_POOL_INVALID_PTR);
    lang_param_assert(ptr != NULL, FX_MEM_POOL_INVALID_PTR);

    fx_sched_lock(&state);
    fx_spl_spinlock_get_from_sched(&pool->lock); 
    rtl_mem_pool_free(&pool->rtl_pool, ptr);
    fx_spl_spinlock_put_from_sched(&pool->lock);
    fx_sched_unlock(state);
    return FX_MEM_POOL_OK;
}

//!
//! Get size of maximum contiguous block.
//! @param pool Initialized memory pool.
//! @param max_chunk Pointer to variable where size of max block will be saved.
//! @return FX_MEM_POOL_OK in case of success, error code otherwise.
//!
int 
fx_mem_pool_get_max_free_chunk(fx_mem_pool_t* pool, size_t* blk_sz)
{
    fx_sched_state_t state;
    lang_param_assert(pool != NULL, FX_MEM_POOL_INVALID_PTR);
    lang_param_assert(blk_sz != NULL, FX_MEM_POOL_INVALID_PTR);
    
    fx_sched_lock(&state);
    fx_spl_spinlock_get_from_sched(&pool->lock);
    *blk_sz = rtl_mem_pool_get_max_blk(&pool->rtl_pool);
    fx_spl_spinlock_put_from_sched(&pool->lock);
    fx_sched_unlock(state);
    return FX_MEM_POOL_OK;
}
