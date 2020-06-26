#ifndef _FX_SEM_V1_HEADER_
#define _FX_SEM_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_sem.h
  *  @brief  Interface of semaphores.
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
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE)

enum
{
    FX_SEM_MAGIC = 0x53454D41, // SEMA
    FX_SEM_OK = FX_STATUS_OK,
    FX_SEM_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_SEM_INVALID_OBJ,
    FX_SEM_UNSUPPORTED_POLICY,
    FX_SEM_INVALID_VALUE,
    FX_SEM_INVALID_TIMEOUT,
    FX_SEM_ERR_MAX
};

//!
//! Semaphore representation.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    lock_t lock;
    unsigned int semaphore;
    unsigned int max_count;
    fx_sync_policy_t policy;
    fx_rtp_t rtp;
    trace_sem_handle_t trace_handle;
} 
fx_sem_t;

int fx_sem_init(
    fx_sem_t* sem, 
    unsigned int init, 
    unsigned int max_val, 
    fx_sync_policy_t p
);
int fx_sem_deinit(fx_sem_t* sem);
int fx_sem_reset(fx_sem_t* sem);
int fx_sem_get_value(fx_sem_t* sem, unsigned int* value);
int fx_sem_post(fx_sem_t* sem);
int fx_sem_post_with_policy(fx_sem_t* sem, fx_sync_policy_t policy);
int fx_sem_wait(fx_sem_t* sem, fx_event_t* event);
int fx_sem_timedwait(fx_sem_t* sem, uint32_t timeout);

FX_METADATA(({ interface: [FX_SEM, V1] }))

#endif
