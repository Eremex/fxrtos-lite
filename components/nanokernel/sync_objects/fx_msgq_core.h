#ifndef _FX_MSGQ_CORE_V1_HEADER_
#define _FX_MSGQ_CORE_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_msgq_core.h
  *  @brief  Interface of core message queue services.
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

#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(TRACE_CORE)
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_SCHED)

//
// Attributes object used to pass additional parameters to test&wait function.
//
typedef struct
{
    bool to_back;
    uintptr_t* buf;
}
fx_msgq_wait_attr_t;

//!
//! Message queue object. 
//! Queue contains two waitable object for senders and receivers.
//!
typedef struct
{
    fx_sync_waitable_t send_wtbl;
    fx_sync_waitable_t recv_wtbl;
    lock_t lock;
    uintptr_t* buf;
    unsigned int items_max;
    unsigned int items;
    unsigned int head;
    unsigned int tail;
    fx_rtp_t rtp;
    fx_sync_policy_t policy;
    trace_queue_handle_t trace_handle;
} 
fx_msgq_t;

#define FX_MSGQ_MAGIC 0x4D534751 // 'MSGQ'
#define fx_msgq_is_valid(msgq) (fx_rtp_check((&((msgq)->rtp)), FX_MSGQ_MAGIC))

int fx_msgq_core_init(
    fx_msgq_t* msgq, 
    uintptr_t* buf, 
    unsigned int sz, 
    fx_sync_policy_t p
);
int fx_msgq_core_deinit(fx_msgq_t* msgq);
int fx_msgq_core_flush(fx_msgq_t* msgq);
bool fx_msgq_test_and_wait_send(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait
);

FX_METADATA(({ interface: [FX_MSGQ_CORE, V1] }))

#endif
