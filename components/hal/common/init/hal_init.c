/** 
  ******************************************************************************
  *  @file   common/init/hal_init.c
  *  @brief  HAL initialization.
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

#include FX_INTERFACE(HAL_INIT)
#include FX_INTERFACE(HAL_ASYNC)
#include FX_INTERFACE(HAL_CPU_INTR)
#include FX_INTERFACE(HW_CPU)

FX_METADATA(({ implementation: [HAL_INIT, STD_LIB] }))

void fx_thread_ctor(void);

//!
//! First thread. This function is called from low-level HAL entry code, 
//! performs system initialization and is used later as idle thread.
//!
void 
fx_kernel_entry(void)
{
    //
    // Microcontroller OS versions use two-stage init: HAL interrupts subsystem
    // and threads module.
    //
    hal_intr_ctor();
    fx_thread_ctor();

    //
    // Lower SPL to level, acceptable for user programs.
    //
    hal_async_lower_spl(SPL_LOW);

    // 
    // Call user application. In normal case this function has to create first
    // user thread.
    //
    fx_app_init();

    //
    // Use current control flow as idle-thread.
    //
    for (;;) hw_cpu_idle();
}
