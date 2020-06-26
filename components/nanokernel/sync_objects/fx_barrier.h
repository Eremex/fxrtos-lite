#ifndef _FX_BARRIER_V1_HEADER_
#define _FX_BARRIER_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_barrier.h
  *  @brief  Interface of barriers.
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
    FX_BARRIER_MAGIC = 0x11334455, // BARR
    FX_BARR_OK = FX_STATUS_OK,
    FX_BARR_INVALID_PTR = FX_THREAD_ERR_MAX,
    FX_BARR_INVALID_OBJ,
    FX_BARR_ZERO_LIMIT,
    FX_BARR_ERR_MAX
};

typedef enum
{
    FX_BARRIER_SERIAL_THREAD = 1
}
fx_barrier_key_t;

//!
//! Barrier representation. 
//!
typedef struct
{
    fx_sync_waitable_t waitable;
    lock_t lock;
    unsigned int barrier;
    unsigned int blocked;
    fx_rtp_t rtp;
} 
fx_barrier_t;

int fx_barrier_init(fx_barrier_t* br, const unsigned int lim);
int fx_barrier_deinit(fx_barrier_t* br);
int fx_barrier_wait(fx_barrier_t* br, fx_barrier_key_t* key, fx_event_t* event);
int fx_barrier_timedwait(fx_barrier_t* br, fx_barrier_key_t* key, uint32_t tm);

FX_METADATA(({ interface: [FX_BARRIER, V1] }))

#endif
