#ifndef _FX_THREAD_APC_LIMITED_HEADER_
#define _FX_THREAD_APC_LIMITED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_thread_apc.h
  *  @brief  Interface of limited version of thread APC subsystem. 
  *          Only uniprocessor systems are supported (internal APC only).
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
#include FX_INTERFACE(FX_DBG)

typedef struct { int dummy; } fx_thread_apc_msg_t;
typedef struct { int dummy; } fx_thread_apc_target_t;

extern void (*fx_thread_apc_on_receive)(fx_thread_apc_target_t*);
#define fx_thread_apc_ctor(idle, cb) fx_thread_apc_on_receive = (cb)
#define fx_thread_apc_target_init(target)
#define fx_thread_apc_msg_init(msg, func, arg)
#define fx_thread_apc_insert(target, msg, accept) (fx_dbg_assert(false), false)
#define fx_thread_apc_cancel(target, msg) false
#define fx_thread_apc_set_mask(new_mask) false
#define fx_thread_apc_pending(target) false
#define fx_thread_apc_deliver(target)

bool fx_thread_apc_insert_internal(
    fx_thread_apc_target_t* target, 
    unsigned int reason, 
    void* arg
);

FX_METADATA(({ interface: [FX_THREAD_APC, LIMITED] }))

#endif
