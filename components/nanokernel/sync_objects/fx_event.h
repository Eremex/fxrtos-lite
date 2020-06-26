#ifndef _FX_EVENT_V1_HEADER_
#define _FX_EVENT_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_event.h
  *  @brief  Interface of simple events.
  *  ***************************************************************************
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
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_SCHED)

enum
{
    FX_EVENT_MAGIC = 0x45564E54, // EVNT
    FX_EVENT_OK = FX_STATUS_OK,
    FX_EVENT_INVALID_PTR = 1,
    FX_EVENT_INVALID_OBJ = 2,
    FX_EVENT_ERR_MAX
};

//!
//! Internal event is embeddable object which does not perform any validation.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    bool state;
    lock_t lock;
} 
fx_event_internal_t;

//!
//! Event representation. 
//!
typedef struct
{
    fx_event_internal_t object;
    fx_rtp_t rtp;
} 
fx_event_t;

#define fx_internal_event_as_waitable(e) (&((e)->waitable))
#define fx_event_as_waitable(e) fx_internal_event_as_waitable((&((e)->object)))
#define fx_event_is_valid(e) (fx_rtp_check((&((e)->rtp)), FX_EVENT_MAGIC))

bool fx_event_test_and_wait(
    fx_sync_waitable_t* object, 
    fx_sync_wait_block_t* wb, 
    const bool wait
);
void fx_event_internal_init(fx_event_internal_t* event, const bool state);
void fx_event_internal_set(fx_event_internal_t* event);
void fx_event_internal_reset(fx_event_internal_t* event);
int fx_event_init(fx_event_t* event, const bool state);
int fx_event_deinit(fx_event_t* event);
int fx_event_set(fx_event_t* event);
int fx_event_reset(fx_event_t* event);
int fx_event_pulse(fx_event_t* event); 
int fx_event_get_state(fx_event_t* event, bool* state);

FX_METADATA(({ interface: [FX_EVENT, V1] }))

#endif
