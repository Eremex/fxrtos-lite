#ifndef _FX_COND_V1_HEADER_
#define _FX_COND_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_cond.h
  *  @brief  Interface of condition variables.
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

#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_EVENT)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_MUTEX)

enum
{
    FX_COND_MAGIC = 0x434F4E44, // COND
    FX_COND_OK = FX_STATUS_OK,
    FX_COND_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_COND_INVALID_OBJ,
    FX_COND_UNSUPPORTED_POLICY,
    FX_COND_INVALID_MUTEX,
    FX_COND_MUTEX_ERROR,
    FX_COND_INVALID_TIMEOUT,
    FX_COND_INVALID_PARAMETER,
    FX_COND_NO_WAITERS,
    FX_COND_ERR_MAX
};

//!
//! Condition variable representation. 
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    fx_rtp_t rtp;
    lock_t lock;
    fx_sync_policy_t policy;
} 
fx_cond_t;

int fx_cond_init(fx_cond_t* cond, const fx_sync_policy_t policy);
int fx_cond_deinit(fx_cond_t* cond);
int fx_cond_signal(fx_cond_t* cond);
int fx_cond_signal_with_policy(fx_cond_t* cond, const fx_sync_policy_t policy);
int fx_cond_broadcast(fx_cond_t* cond);
int fx_cond_wait(fx_cond_t* cond, fx_mutex_t* mutex, fx_event_t* cancel_event);
int fx_cond_timedwait(fx_cond_t* cond, fx_mutex_t* mutex, uint32_t tout);

FX_METADATA(({ interface: [FX_COND, V1] }))

#endif
