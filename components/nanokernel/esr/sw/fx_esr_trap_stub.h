#ifndef _FX_ESR_TRAP_STUB_HEADER_
#define _FX_ESR_TRAP_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_esr_trap_stub.h
  *  @brief  Stub interface header for disabled exception handling in ESRs.
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
#include FX_INTERFACE(FX_DBG)  

//
// Stubs for HAL exceptions.
//
enum
{
    HAL_SIG_FPE = 0,
    HAL_SIG_TRAP,
    HAL_SIG_ILL,
    HAL_SIG_SEGV,
    HAL_SIG_COUNT
};

//!
//! Exception aspect of ESR. 
//!
typedef struct { int dummy; } fx_esr_exception_target_t;

typedef void (*fx_esr_exc_handler_t)(
  fx_esr_exception_target_t* t, 
  unsigned int exc_id, 
  void* arg
);

#define fx_esr_exception_set_handler(exc_id, handler, old) (true)
#define fx_esr_exception_target_init(target)
#define fx_esr_exception_send(exc_id, stack_frame) \
    fx_panic("Exception handling is disabled");
#define fx_esr_exception_check(target)

FX_METADATA(({ interface: [FX_ESR_TRAP, STUB] }))

#endif
