#ifndef _FX_MUTEX_V1_HEADER_
#define _FX_MUTEX_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_mutex.h
  *  @brief  Interface of mutexes.
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
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE) 

enum
{
    FX_MUTEX_MAGIC = 0x4D555458, // MUTX
    FX_MUTEX_OK = FX_STATUS_OK,
    FX_MUTEX_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_MUTEX_INVALID_OBJ,
    FX_MUTEX_UNSUPPORTED_POLICY,
    FX_MUTEX_INVALID_PRIORITY,
    FX_MUTEX_INVALID_TIMEOUT,
    FX_MUTEX_WRONG_OWNER,
    FX_MUTEX_RECURSIVE_LIMIT,
    FX_MUTEX_ABANDONED,
    FX_MUTEX_ERR_MAX
};

//!
//! Mutex object.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    lock_t lock;
    fx_thread_t* volatile owner;
    volatile uint_fast16_t recursive_locks;
    bool ceiling_enabled;
    fx_sched_params_t ceiling_orig;
    fx_sched_params_t owner_params;
    fx_sync_policy_t policy;
    fx_rtp_t rtp;
    trace_mutex_handle_t trace_handle;
} 
fx_mutex_t;

#define fx_mutex_limit_exceeded(m) ((m)->recursive_locks == UINT_FAST16_MAX)
#define fx_mutex_lock_counter_get(m) ((m)->recursive_locks)
#define fx_mutex_lock_counter_set(m, c) ((m)->recursive_locks = (c))
#define FX_MUTEX_CEILING_DISABLED (~0U)

int fx_mutex_init(fx_mutex_t* mutex, unsigned int prio, fx_sync_policy_t p);
int fx_mutex_deinit(fx_mutex_t* mutex);
int fx_mutex_acquire(fx_mutex_t* mutex, fx_event_t* event);
int fx_mutex_timedacquire(fx_mutex_t* mutex, uint32_t tout);
int fx_mutex_release(fx_mutex_t* mutex);
int fx_mutex_release_with_policy(fx_mutex_t* mutex, fx_sync_policy_t policy);
fx_thread_t* fx_mutex_get_owner(fx_mutex_t* mutex);

FX_METADATA(({ interface: [FX_MUTEX, V1] }))

#endif
