#ifndef _FX_DPC_STUB_HEADER_
#define _FX_DPC_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   unified/fx_dpc.h
  *  @brief  DPC stub.
  *  Deferred procedures work in context of ISR.
  *  This DPC implementation is intended to be used with unified interrupt 
  *  architecture.
  *  DPC request results in direct call of deferred function instead of putting 
  *  it into queue. Because inserting of DPC is usually performed in ISR,
  *  deferred function may be called at SPL greater than DISPATCH.
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
  
typedef struct { int dummy; } fx_dpc_t;

#define fx_dpc_ctor()
#define fx_dpc_init(dpc)
#define fx_dpc_request(dpc, func, arg) (func(dpc, arg), true)
#define fx_dpc_cancel(dpc) (false)
#define fx_dpc_set_target_cpu(dpc, cpu) fx_dbg_assert(cpu == 0)
#define fx_dpc_environment() (false)
#define fx_dpc_handle_queue()

FX_METADATA(({ interface: [FX_DPC, STUB] }))

#endif
