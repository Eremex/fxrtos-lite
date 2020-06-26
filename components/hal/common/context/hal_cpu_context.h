#ifndef _HAL_CPU_CONTEXT_KER_FRAME_BASED_HEADER_
#define _HAL_CPU_CONTEXT_KER_FRAME_BASED_HEADER_

/** 
  ******************************************************************************
  *  @file common/context/hal_cpu_context.h
  *  @brief CPU context management functions.
  *  This module may be used for any HAL without usermode threads support.
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

struct _hal_intr_frame_t;

//!
//! Hardware context contains only pointer to the current stack frame containing
//! full register state.
//!
typedef struct
{
    struct _hal_intr_frame_t* frame;  
}
hal_cpu_context_t;

void hal_context_ker_create(
    hal_cpu_context_t* context, 
    uintptr_t stk, 
    uintptr_t entry, 
    uintptr_t arg
);

#define hal_context_ker_delete(context)
void hal_context_switch(hal_cpu_context_t* new_ctx, hal_cpu_context_t* old_ctx);

FX_METADATA(({ interface: [HAL_CPU_CONTEXT, KER_FRAME_BASED] }))

#endif
