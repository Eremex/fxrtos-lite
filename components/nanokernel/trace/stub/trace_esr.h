#ifndef _TRACE_ESR_STUB_HEADER_
#define _TRACE_ESR_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   trace_esr.h
  *  @brief  Stub for trace subsystem. Disables ESR tracing.
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

typedef struct {int dummy;} trace_esr_handle_t;

#define trace_esr_init_idle(esr_handle, epl) ((void)(esr_handle))
#define trace_esr_init(esr_handle, epl) ((void)(esr_handle))
#define trace_esr_deinit(esr_handle, epl) ((void)(esr_handle))
#define trace_esr_activate(me_handle, esr_handle) ((void)(me_handle))
#define trace_esr_exit(me_handle, esr_handle) ((void)(me_handle))
#define trace_esr_raise_epl(me_handle, new_epl) ((void)(me_handle))
#define trace_esr_lower_epl(me_handle, new_epl) ((void)(me_handle))
#define trace_esr_preemption(me_handle, next_handle) ((void)(me_handle))

FX_METADATA(({ interface: [TRACE_ESR, STUB] }))

#endif
