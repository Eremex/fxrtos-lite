#ifndef _FX_EV_FLAGS_V1_HEADER_
#define _FX_EV_FLAGS_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_ev_flags.h
  *  @brief  Interface of event flags.
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
#include FX_INTERFACE(FX_SYNC)
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_THREAD)
#include FX_INTERFACE(RTL_QUEUE)

//!
//! Error codes.
//! 
enum
{
    FX_EV_FLAGS_MAGIC = 0x45564600, // EVF
    FX_EV_FLAGS_OK = FX_STATUS_OK,
    FX_EV_FLAGS_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_EV_FLAGS_INVALID_OBJ,
    FX_EV_FLAGS_INVALID_FLAGS,
    FX_EV_FLAGS_INVALID_OPTIONS,
    FX_EV_FLAGS_ERR_MAX
};

//!
//! Wait options.
//! 
enum
{
    FX_EV_FLAGS_OR = 0,     //!< Waiting any flag from specified set to be 1.
    FX_EV_FLAGS_AND = 1,    //!< Waitinf all flags from specified set to be 1.
    FX_EV_FLAGS_CLEAR = 2,  //!< Consume flags which make wait satisfied.
};

//!
//! Event flags representation. 
//! It ises temporary queue for items to be notified.
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    rtl_queue_t temp;
    uint_fast32_t flags;
    fx_rtp_t rtp;
    lock_t lock;
} 
fx_ev_flags_t;

int fx_ev_flags_init(fx_ev_flags_t* evf);
int fx_ev_flags_deinit(fx_ev_flags_t* evf);
int fx_ev_flags_wait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state, 
    fx_event_t* cancel_ev
);
int fx_ev_flags_timedwait(
    fx_ev_flags_t* evf, 
    const uint_fast32_t req_flags, 
    const unsigned int option, 
    uint_fast32_t* state, 
    uint32_t tout
);
int fx_ev_flags_set(fx_ev_flags_t* evf, uint_fast32_t flags, bool type);

FX_METADATA(({ interface: [FX_EV_FLAGS, V1] }))

#endif
