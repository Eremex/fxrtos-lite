#ifndef _FX_ESR_SW_HEADER_
#define _FX_ESR_SW_HEADER_

/** 
  ******************************************************************************
  *  @file   sw/fx_esr.h
  *  @brief  Interface header for event service routines.
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

#include FX_INTERFACE(LANG_TYPES) 
#include FX_INTERFACE(HAL_INTR_FRAME) 
#include FX_INTERFACE(FX_RTP)
#include FX_INTERFACE(FX_SCHED) 
#include FX_INTERFACE(TRACE_ESR) 
#include FX_INTERFACE(FX_SYNC) 
#include FX_INTERFACE(FX_ESR_TRAP)

//!
//! Datatype for ESR priority level (EPL). It is like SPL, but it is attribute 
//! of ESR, whereas SPL is attribute of processor. EPL may be considered as 
//! sub-levels of SPL_LOW level.
//!
typedef unsigned int epl_t;

enum
{
    FX_ESR_OK = FX_STATUS_OK,
    FX_ESR_INVALID_PTR = 1,
    FX_ESR_INVALID_EPL = 2,
    FX_ESR_INVALID_OBJ = 3,
    FX_ESR_INVALID_ENTRY = 4,
    FX_ESR_ERR_MAX
};

//!
//! ESR representation.
//!
typedef struct _fx_esr_t
{
    epl_t epl;
    fx_rtp_t rtp;
    fx_sched_item_t sched_item;
    bool active;
    fx_esr_exception_target_t traps;
    hal_intr_frame_t* intr_context;
    void (*func)(struct _fx_esr_t*, uintptr_t, void*);
    void* arg;
    trace_esr_handle_t trace_handle;
    uintptr_t msg;
    fx_sync_waiter_t waiter;
    fx_sync_wait_block_t wb;
}
fx_esr_t;

//
// HAL uses this function as "kernel init", in case when "threads" are 
// implemented as other executable and schedulable entity they still have to
// name init function as thread_ctor.
//
void fx_thread_ctor(void);

int fx_esr_init(
    fx_esr_t* esr, 
    epl_t epl, 
    void (*func)(fx_esr_t*, uintptr_t, void*), 
    void* arg
);

int fx_esr_deinit(fx_esr_t* esr);
int fx_esr_activate(fx_esr_t* esr, uintptr_t msg);
void fx_esr_exit(void);
#define fx_esr_get_pl(esr) (esr->epl)
epl_t fx_esr_raise_pl(epl_t new_epl);
void fx_esr_lower_pl(epl_t new_epl);
int fx_esr_wait_msg(fx_sync_waitable_t* object, bool wait);

FX_METADATA(({ interface: [FX_ESR, SW] }))

#endif
