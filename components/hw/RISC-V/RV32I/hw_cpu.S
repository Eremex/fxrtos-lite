;/** 
; ******************************************************************************
; *  @file   RV32I/hw_cpu.S
; *  @brief  Low-level utilities and CPU-specific functions.      
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
;// IAR assembler can not use function-like macros in includes.
;// So, in case when fx_interface macro is not defined, it always mean that the
;// project is being built from some IDE, just use appropriate filename.
;//
#ifndef FX_INTERFACE
#include <LANG_ASM.h>
#include <CFG_OPTIONS.h>
#else
#include FX_INTERFACE(LANG_ASM)
#include FX_INTERFACE(CFG_OPTIONS)
#endif

;FX_METADATA(({ implementation: [HW_CPU, RV32I] }))

ASM_ENTRY1(hw_cpu_dmb)
    fence
    ret

ASM_ENTRY1(hw_cpu_atomic_cas)
    fence
    csrr    t0, mstatus
    csrc    mstatus, RV_SPEC_MSTATUS_MIE
    lw      t1, (a0)
    bne     t1, a1, 1f
    sw      a2, (a0)
1:
    csrw    mstatus, t0
    fence
    mv      a0, t1
    ret

ASM_ENTRY1(hw_cpu_atomic_swap)
    fence
    csrr    t0, mstatus
    csrc    mstatus, RV_SPEC_MSTATUS_MIE
    lw      t1, (a0)
    sw      a1, (a0)
    csrw    mstatus, t0
    fence
    mv      a0, t1
    ret

ASM_ENTRY1(hw_cpu_atomic_add)
    fence
    csrr    t0, mstatus
    csrc    mstatus, RV_SPEC_MSTATUS_MIE
    lw      t1, (a0)
    add     a1, a1, t1
    sw      a1, (a0)
    csrw    mstatus, t0
    fence
    mv      a0, t1
    ret

ASM_ENTRY1(hw_cpu_clz)
    mv      a1, a0
    li	    a2, 0x80000000
    li      a0, 0
1:
    and	    a3, a2, a1
    bnez    a3, 2f
    addi    a0, a0, 1
    srli    a2, a2, 1
    bnez    a2, 1b
2:
    ret

ASM_ENTRY1(hw_cpu_ctz)
	mv		a1, a0
    li	    a2, 1
    li      a0, 0
1:
    and	    a3, a2, a1
    bnez    a3, 2f
    addi    a0, a0, 1
    slli    a2, a2, 1
    bnez    a2, 1b
2:
    ret

ASM_ENTRY1(hw_cpu_idle)
    wfi
    ret

ASM_ENTRY1(hw_cpu_intr_enable)
    csrs    mstatus, RV_SPEC_MSTATUS_MIE
    ret

ASM_ENTRY1(hw_cpu_intr_disable)
    csrci   mstatus, RV_SPEC_MSTATUS_MIE
    ret
