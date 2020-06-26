/** 
  ******************************************************************************
  *  @file   common/context/hal_cpu_context.c
  *  @brief  CPU context management functions.          
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

#include FX_INTERFACE(HAL_CPU_CONTEXT)
#include FX_INTERFACE(HAL_INTR_FRAME)

FX_METADATA(({ implementation: [HAL_CPU_CONTEXT, KER_FRAME_BASED] }))

//!
//! Context initialization.
//! Creating new context frame on the stack.
//! @param [in] context Pointer to context structure to be initialized.
//! @param [in] stack Kernel stack pointer to be used in new thread.
//! @param [in] entry Entry point for the new thread.
//! @param [in] arg Argument to be passed in entry-point function.
//!
void 
hal_context_ker_create(
    hal_cpu_context_t* context, 
    uintptr_t stack, 
    uintptr_t entry, 
    uintptr_t arg)
{
    hal_intr_frame_t* frame = hal_intr_frame_alloc( (hal_intr_frame_t*)stack );

    hal_intr_frame_modify(frame, KER_FRAME_ENTRY, entry);
    hal_intr_frame_modify(frame, KER_FRAME_ARG0, arg);

    context->frame = frame;  
}

//!
//! Context switching.
//! Context switching on microcontroller platform only switches current 
//! interrupt frame pointer. Context switch may be called ONLY in context of
//! dispatch interrupt handler. 
//! @param [in] new_ctx New context to be set.
//! @param [out] old_ctx Previous context (context of interrupted thread).
//!
void 
hal_context_switch(hal_cpu_context_t* new_ctx, hal_cpu_context_t* old_ctx)
{
    old_ctx->frame = hal_intr_frame_switch(new_ctx->frame);
}
