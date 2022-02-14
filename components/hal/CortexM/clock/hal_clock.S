;/** 
; ******************************************************************************
; *  @file   CortexM/clock/hal_clock.S
; *  @brief  Systick interrupt handler.
; *
; ******************************************************************************
; *  Copyright (C) JSC EREMEX, 2008-2020.
; *  Redistribution and use in source and binary forms, with or without 
; *  modification, are permitted provided that the following conditions are met:
; *  1. Redistributions of source code must retain the above copyright notice,
; *     this list of conditions and the following disclaimer.
; *  2. Redistributions in binary form must reproduce the above copyright 
; *     notice, this list of conditions and the following disclaimer in the 
; *     documentation and/or other materials provided with the distribution.
; *  3. Neither the name of the copyright holder nor the names of its 
; *     contributors may be used to endorse or promote products derived from 
; *     this software without specific prior written permission.
; *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
; *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
; *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
; *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
; *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
; *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
; *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
; *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
; *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
; *  POSSIBILITY OF SUCH DAMAGE.
; *****************************************************************************/

;//
;// Keil assembler does not allow functional macros in command line.
;// IAR assembler can not use function-like macros in includes.
;// So, in case when fx_interface macro is not defined, it always mean building
;// project from some IDE, just use appropriate filename.
;//
#ifndef FX_INTERFACE
#include <LANG_ASM.h>
#include <CFG_OPTIONS.h>
#else
#include FX_INTERFACE(LANG_ASM)
#include FX_INTERFACE(CFG_OPTIONS)
#endif

;FX_METADATA(({ implementation: [HAL_CLOCK, ARMv7M_V1] }))

  EXTERN_FUNC(hal_intr_frame_save)
  EXTERN_FUNC(hal_intr_frame_restore)

#if defined HAL_CLOCK_TICK_HOOK  
  EXTERN_FUNC(fx_app_tick)
#else  
  EXTERN_FUNC(fx_tick_handler)
#endif

ASM_ENTRY1(hal_tick_entry)
  ASM_ENTRY2(hal_tick_entry)
    push  {lr}                   ;// LR is saved onto the MAIN stack.
    bl    hal_intr_frame_save     

#if defined HAL_CLOCK_TICK_HOOK  
    bl    fx_app_tick
#else  
    bl    fx_tick_handler
#endif

    bl   hal_intr_frame_restore
    pop  {pc}
  ENDF 

    ENDFILE
