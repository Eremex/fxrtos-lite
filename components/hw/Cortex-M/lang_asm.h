#ifndef _LANG_ASM_GCC_M0_HEADER_
#define _LANG_ASM_GCC_M0_HEADER_

/** 
  ******************************************************************************
  *  @file   Cortex-M/lang_asm.h
  *  @brief  GNU Assembler definitions for Cortex M.        
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

;FX_METADATA(({ interface: [LANG_ASM, ARMv7M] }))

#if defined __GNUC__

.syntax unified
.thumb

#define label(name) name:
#define ASM_ENTRY1(fn) .text; .align 2; .global fn; .type fn,%function; .thumb_func; fn:
#define ASM_ENTRY2(fn)
#define ENDF
#define ENDFILE
#define EXTERN_FUNC(name)

#elif defined __CC_ARM

  PRESERVE8
  THUMB
  AREA |.text|, CODE, READONLY

#define ASM_ENTRY1(fn) fn PROC
#define ASM_ENTRY2(fn) EXPORT fn
#define label(name) name        
#define ENDF ENDP
#define ENDFILE END
#define EXTERN_FUNC(fn) IMPORT fn

#elif (defined __IAR_SYSTEMS_ASM__) || (defined __ICCARM__)

  SECTION `.text`:CODE:NOROOT(2)
  THUMB

#define ASM_ENTRY1(fn) fn:
#define ASM_ENTRY2(fn) PUBLIC fn
#define label(name) name:
#define ENDF
#define ENDFILE END
#define EXTERN_FUNC(fn) EXTERN fn

#else 
#error Unknown assembler (only GCC, Keil and IAR are supported).
#endif

#endif
