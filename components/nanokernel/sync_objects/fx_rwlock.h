#ifndef _FX_RWLOCK_V1_HEADER_
#define _FX_RWLOCK_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_rwlock.h
  *  @brief  Interface of read/write lock.
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

#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)

enum
{
    FX_RWLOCK_MAGIC = 0x52574C4B, //RWLK
    FX_RWLOCK_OK = FX_STATUS_OK,
    FX_RWLOCK_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_RWLOCK_INVALID_OBJ,
    FX_RWLOCK_UNSUPPORTED_POLICY,
    FX_RWLOCK_INVALID_TIMEOUT,
    FX_RWLOCK_ERR_MAX
};

//!
//! RW-lock object. 
//! There are two wait queues used, one for readers and one for writers.
//!
typedef struct
{
    fx_sync_waitable_t rd_wtbl;
    fx_sync_waitable_t wr_wtbl;
    lock_t lock;
    fx_rtp_t rtp;
    unsigned int readers;
    fx_thread_t* owner;
    fx_sync_policy_t policy;
} 
fx_rwlock_t;

int fx_rwlock_init(fx_rwlock_t* rw, fx_sync_policy_t policy);
int fx_rwlock_deinit(fx_rwlock_t* rw);
int fx_rwlock_rd_timedlock(fx_rwlock_t* rw, uint32_t tout);
int fx_rwlock_wr_timedlock(fx_rwlock_t* rw, uint32_t tout);
int fx_rwlock_rd_lock(fx_rwlock_t* rw, fx_event_t* cancel_event);
int fx_rwlock_wr_lock(fx_rwlock_t* rw, fx_event_t* cancel_event);
int fx_rwlock_unlock(fx_rwlock_t* rw);
int fx_rwlock_unlock_with_policy(fx_rwlock_t* rw, fx_sync_policy_t policy);

FX_METADATA(({ interface: [FX_RWLOCK, V1] }))

#endif
