#ifndef _HAL_ASYNC_ARMv7M_UNIFIED_HEADER_
#define _HAL_ASYNC_ARMv7M_UNIFIED_HEADER_

/** 
  ******************************************************************************
  *  @file   CortexM/sync/unified/hal_async.h
  *  @brief  SPL management functions for unified interrupt architecture.
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

FX_METADATA(({ interface: [HAL_ASYNC, ARMv7M_UNIFIED] }))

//!
//! Constants for SPL levels. These constants are tightly coupled with PRIMASK 
//! register format, so, they must not be changed!
//!
typedef enum
{
    SPL_SYNC = 0x01,
    SPL_DISPATCH = SPL_SYNC,
    SPL_LOW = 0x00
}
spl_t;

#define hal_async_ctor()

spl_t hal_async_raise_spl(const spl_t new_spl); 
void hal_async_lower_spl(const spl_t new_spl);
spl_t hal_async_get_current_spl(void);

#define ICSR_ADDR ((volatile unsigned int*) 0xE000ED04)
#define hal_async_request_swi(spl) ((*ICSR_ADDR) = 0x10000000)
                
#endif
