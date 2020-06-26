#ifndef _FX_MEM_POOL_TLSF_HEADER_
#define _FX_MEM_POOL_TLSF_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_mem_pool.h
  *  @brief  Wrapper for memory pool.
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

#include FX_INTERFACE(LANG_TYPES)
#include FX_INTERFACE(FX_SPL)
#include FX_INTERFACE(RTL_MEM_POOL)

//
// Error codes.
//
enum
{
    FX_MEM_POOL_OK = FX_STATUS_OK,
    FX_MEM_POOL_INVALID_PTR,
    FX_MEM_POOL_INVALID_OBJ,
    FX_MEM_POOL_INVALID_BUF,
    FX_MEM_POOL_ZERO_SZ,
    FX_MEM_POOL_NO_MEM,
    FX_MEM_POOL_ERR_MAX
};

//!
//! Bytes pool structure.
//!
typedef struct 
{
    lock_t lock;
    rtl_mem_pool_t rtl_pool;
} 
fx_mem_pool_t;

int fx_mem_pool_init(fx_mem_pool_t* pool);
int fx_mem_pool_deinit(fx_mem_pool_t* pool);
int fx_mem_pool_add_mem(fx_mem_pool_t* pool, uintptr_t mem, size_t bytes);
int fx_mem_pool_alloc(fx_mem_pool_t* pool, size_t bytes, void** p);
int fx_mem_pool_free(fx_mem_pool_t* pool, void* ptr);
int fx_mem_pool_get_max_free_chunk(fx_mem_pool_t* pool, size_t* blk_sz);

FX_METADATA(({ interface: [FX_MEM_POOL, TLSF] }))

#endif
