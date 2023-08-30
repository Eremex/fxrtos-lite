/** 
  ******************************************************************************
  *  @file   RISCV32I/intr/hal_cpu_intr0.S
  *  @brief  HAL interrupt implementation for RISCV.
  *
  ******************************************************************************
  *  Copyright (C) JSC EREMEX, 2020-2023.
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
  
#include FX_INTERFACE(CFG_OPTIONS)
#include FX_INTERFACE(LANG_ASM)

;FX_METADATA(({ implementation: [HAL_CPU_INTR, RV32I] }))

#define CONTEXT_SIZE (29 * 4)

#define MOVE____GPRS(cmd)  \
    cmd  ra,  4(sp);   \
    cmd  x5,  8(sp);   \
    cmd  x6,  12(sp);  \
    cmd  x7,  16(sp);  \
    cmd  x28, 20(sp);  \
    cmd  x29, 24(sp);  \
    cmd  x30, 28(sp);  \
    cmd  x31, 32(sp);  \
    cmd  x10, 36(sp);  \
    cmd  x11, 40(sp);  \
    cmd  x12, 44(sp);  \
    cmd  x13, 48(sp);  \
    cmd  x14, 52(sp);  \
    cmd  x15, 56(sp);  \
    cmd  x16, 60(sp);  \
    cmd  x17, 64(sp);  \
    cmd  x8,  68(sp);  \
    cmd  x9,  72(sp);  \
    cmd  x18, 76(sp);  \
    cmd  x19, 80(sp);  \
    cmd  x20, 84(sp);  \
    cmd  x21, 88(sp);  \
    cmd  x22, 92(sp);  \
    cmd  x23, 96(sp);  \
    cmd  x24, 100(sp); \
    cmd  x25, 104(sp); \
    cmd  x26, 108(sp); \
    cmd  x27, 112(sp);  

.align 4
ASM_ENTRY1(hal_intr_entry)
    addi    sp, sp, -CONTEXT_SIZE
    MOVE____GPRS(sw)
    csrr    t0, mepc
    sw      t0, 0(sp)
    csrr    a0, mcause
    srli    a1, a0, 31
    beqz    a1, synchronous_trap
    la      t0, g_hal_intr_nesting
    lw      t1, 0(t0)
    addi    t2, t1, 1
    sw      t2, 0(t0)
    bnez    t1, asynchronous_intr
    la      t0, g_hal_intr_stack_frame
    sw      sp, 0(t0)
    la      sp, g_hal_intr_stack
    addi    sp, sp, HAL_INTR_STACK_SIZE
asynchronous_intr:
    jal     hal_intr_handler
    la      t0, g_hal_intr_nesting
    lw      t1, 0(t0)
    addi    t1, t1, -1
    sw      t1, 0(t0)
    bnez    t1, context_restore
    la      t0, g_hal_intr_stack_frame
    lw      sp, 0(t0)
context_restore:
    li      t0, (RV_SPEC_MSTATUS_MPP_M | RV_SPEC_MSTATUS_MPIE)
    csrs    mstatus, t0
    lw      t0, 0(sp)
    csrw    mepc, t0
    MOVE____GPRS(lw)
    addi    sp, sp, CONTEXT_SIZE
    RV_SPEC_INT_RET
synchronous_trap:
    jal     hal_trap_handler
    j       context_restore


ASM_ENTRY1(hal_intr_check_swi)
    addi    sp, sp, -CONTEXT_SIZE
    MOVE____GPRS(sw)
    sw      ra, 0(sp)
    la      t0, g_hal_intr_nesting
    li      t1, 1
    sw      t1, 0(t0)
    la      t0, g_hal_intr_stack_frame
    sw      sp, 0(t0)
    la      sp, g_hal_intr_stack
    addi    sp, sp, HAL_INTR_STACK_SIZE
    jal     hal_swi_handler
    la      t0, g_hal_intr_nesting
    sw      zero, 0(t0)
    la      t0, g_hal_intr_stack_frame
    lw      sp, 0(t0)
    lw      t0, 0(sp)
    csrw    mepc, t0
    li      t0, (RV_SPEC_MSTATUS_MPP_M | RV_SPEC_MSTATUS_MPIE)
    csrs    mstatus, t0
    MOVE____GPRS(lw)
    addi    sp, sp, CONTEXT_SIZE
    RV_SPEC_INT_RET
