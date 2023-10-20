#ifndef _CFG_OPTIONS_RV32I_GNU_HEADER_
#define _CFG_OPTIONS_RV32I_GNU_HEADER_

/** 
  ******************************************************************************
  *  @file   standard-rv32i-v1.10-gnu-options.h
  *  @brief  Kernel options.
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

#define FX_SCHED_ALG_PRIO_NUM 32
#define FX_TIMER_THREAD_PRIO 1                              
#define FX_TIMER_THREAD_STACK_SIZE 0x400
#define HAL_INTR_TIMER_MCAUSE 0x80000007
#define HAL_INTR_MCAUSE_EXCCODE_MASK 0x80000FFF
#define HAL_INTR_STACK_SIZE 0x400
#define RTL_MEM_POOL_MAX_CHUNK 15

#define RV_SPEC_MSTATUS_MPP_M (3 << 11)
#define RV_SPEC_MSTATUS_MPIE (1 << 7)
#define RV_SPEC_MSTATUS_MIE 8
#define RV_SPEC_INT_RET	mret

FX_METADATA(({ interface: [CFG_OPTIONS, STANDARD_RV32I_GNU] }))

#endif
