;/** 
; ******************************************************************************
; *  @file   CortexM/intr_v7m_fpu/hal_cpu_intr.S
; *  @brief  HAL interrupt implementation for CortexM4F.
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
#else
#include FX_INTERFACE(LANG_ASM)
#endif

;FX_METADATA(({ implementation: [HAL_CPU_INTR, ARMv7M_FPU] }))

;//
;// Saves r4-r11 registers onto the stack.
;// It is expected that PSP already points to hardware-supplied part of frame.
;//
ASM_ENTRY1(hal_intr_frame_save)
  ASM_ENTRY2(hal_intr_frame_save)
    stmdb sp!, {r4-r11}
    bx    lr
  ENDF

;//
;// Loads r4-r11 registers from the stack.
;//
ASM_ENTRY1(hal_intr_frame_restore)
  ASM_ENTRY2(hal_intr_frame_restore)
    ldmia sp!, {r4-r11}
    bx    lr
  ENDF

;//
;// Low-level interrupt entry. This function should be installed into vector
;// table for all OS-managed hw vectors.
;//
  EXTERN_FUNC(fx_intr_handler)
ASM_ENTRY1(hal_intr_entry)
  ASM_ENTRY2(hal_intr_entry)
    push  {lr}
    bl    hal_intr_frame_save
    bl    fx_intr_handler
    bl    hal_intr_frame_restore
    pop   {pc}
  ENDF

;//
;// PendSV low-level entry.
;//
  EXTERN_FUNC(fx_dispatch_handler)
ASM_ENTRY1(hal_swi_entry)
  ASM_ENTRY2(hal_swi_entry)
    mrs       r0, psp
    tst       lr, #0x10
    it        eq
    vstmdbeq  r0!, {s16-s31}
    stmdb     r0!, {r4-r11, lr}
    msr       psp, r0
    isb                     
    bl        fx_dispatch_handler
    mrs       r0, psp
    ldmia     r0!, {r4-r11, lr}
    tst       lr, #0x10
    it        eq
    vldmiaeq  r0!, {s16-s31}    
    msr       psp, r0
    isb
    bx        lr
  ENDF

;//
;// Returns current IRQ vector when called in interrupt handler.
;//
ASM_ENTRY1(hal_intr_get_current_vect)
  ASM_ENTRY2(hal_intr_get_current_vect)
    mrs   r0, ipsr
    sub   r0, r0, #16
    bx    lr
  ENDF
  
    ENDFILE
