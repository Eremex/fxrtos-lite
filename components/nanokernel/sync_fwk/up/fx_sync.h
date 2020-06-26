#ifndef _FX_SYNC_UP_QUEUE_HEADER_
#define _FX_SYNC_UP_QUEUE_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_sync.h
  *  @brief  Basic synchronization layer. 
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
#include FX_INTERFACE(RTL_QUEUE)
#include FX_INTERFACE(FX_SCHED_ALG)

struct _fx_sync_waiter_t;
struct _fx_sync_waitable_t;
struct _fx_sync_wait_block_t;
typedef struct _fx_sync_waiter_t fx_sync_waiter_t;
typedef struct _fx_sync_waitable_t fx_sync_waitable_t;
typedef struct _fx_sync_wait_block_t fx_sync_wait_block_t;

//!
//! Scheduling policy for waitable object queue.
//! FIFO means that waiters will be released in FIFO order, PRIO: in priority
//! order.
//!
typedef enum
{
    FX_SYNC_POLICY_FIFO = 0,
    FX_SYNC_POLICY_PRIO = 1,
    FX_SYNC_POLICY_MAX,
    FX_SYNC_POLICY_DEFAULT = FX_SYNC_POLICY_FIFO
}
fx_sync_policy_t;

//!
//! Status of wait operation.
//!
typedef enum
{
    FX_WAIT_IN_PROGRESS = 0,
    FX_WAIT_SATISFIED   = 1,
    FX_WAIT_CANCELLED   = 2,
    FX_WAIT_DELETED     = 3,
    FX_WAIT_STATUS_MAX
}
fx_wait_status_t;

//!
//! Waitable object representation. 
//! It is base class for all synchronization primitives and contains queue of
//! wait objects. The test function atomically tests the object and inserts 
//! block to the queue if the object is in nonsignaled state.
//!
struct _fx_sync_waitable_t
{
    rtl_queue_t wq;          
    bool (*test_wait)(fx_sync_waitable_t*, fx_sync_wait_block_t*, const bool);
};

//!
//! Base class of waiter.
//! N.B. Waiter methods is NOT thread safe. It is expected that waiter is a
//! thread and therefore all waiter methods are called in context of one 
//! thread (sequentially).
//!
struct _fx_sync_waiter_t
{
    fx_sched_params_t* sched_params;
    fx_sync_wait_block_t* wb;
    unsigned int wb_num;
};

//!
//! Representation of wait block. 
//! Wait block is a link between waiter and waitable.
//! Attribute is used to pass some info from waiter to primitive's logic. 
//! It is also used in order to return object-specific info from wait functions.
//! N.B. Attribute is used before wait starts, so, it may never be used 
//! simultaneously with status, which indicates status of notification, 
//! therefore they may reside in same memory location. 
//! Warning! Do not use values reserved for status as attributes! In case when 
//! it is needed to implement primitive's logic use indirect parameter passing 
//! (when attribute is a pointer to memory location holding actual value).
//!
struct _fx_sync_wait_block_t
{
    fx_sync_waiter_t* waiter;
    fx_sync_waitable_t* waitable;
    union
    {
        void* attribute;
        fx_wait_status_t status;
    } u;                                        
    rtl_queue_linkage_t link;
};

#define fx_sync_waitable_lock(w)
#define fx_sync_waitable_unlock(w)
#define _fx_sync_waitable_nonempty(w) (!rtl_queue_empty(&((w)->wq)))
#define fx_sync_waitable_as_queue(w) (&((w)->wq))
#define fx_sync_waiter_init(w, params) (w)->sched_params = params
#define fx_sync_is_waiter_satisfied(w) false
#define fx_sync_wb_as_queue_item(wb) (&((wb)->link))
#define fx_sync_queue_item_as_wb(item) \
    (rtl_queue_entry(item, fx_sync_wait_block_t, link))
#define fx_sync_wait_block_get_status(wb) ((wb)->u.status)
#define fx_sync_wait_block_get_attr(wb) ((wb)->u.attribute)
#define FX_SYNC_WAIT_BLOCK_INITIALIZER(wtr, wtbl, attr) \
    {(wtr), NULL, {attr}, RTL_QUEUE_INITIALIZER}

//!
//! Preparing waiter for new wait operation. Should be perfermed before every 
//! wait operation. This implementation supports only waits by OR, so, expected 
//! notification number is always 1 and therefore unused.
//!
static inline void 
fx_sync_waiter_prepare(
    fx_sync_waiter_t* waiter, 
    fx_sync_wait_block_t* wb_array, 
    unsigned int wb_n, 
    unsigned int expected) 
{
    waiter->wb = wb_array;
    waiter->wb_num = wb_n;
}

void fx_sync_waitable_init(
    fx_sync_waitable_t* waitable, 
    void* ignored,
    bool (*fn)(fx_sync_waitable_t*, fx_sync_wait_block_t*, const bool)
);

fx_sync_wait_block_t* _fx_sync_wait_block_get(
    fx_sync_waitable_t* waitable, 
    fx_sync_policy_t p
);

void _fx_sync_wait_notify(
    fx_sync_waitable_t* waitable, 
    fx_wait_status_t s, 
    fx_sync_wait_block_t* wb
);

void _fx_sync_wait_start(fx_sync_waitable_t* w, fx_sync_wait_block_t* wb);
unsigned int fx_sync_wait_rollback(fx_sync_waiter_t* waiter);
extern void fx_sync_waiter_notify(fx_sync_waiter_t* waiter);

FX_METADATA(({ interface: [FX_SYNC, UP_QUEUE] }))

#endif
