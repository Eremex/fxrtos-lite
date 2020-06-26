#ifndef _LANG_TYPES_OS_186_HEADER_
#define _LANG_TYPES_OS_186_HEADER_

/** 
  ******************************************************************************
  *  @file   lang_types.h
  *  @brief  Common language extensions, default headers and useful macros.
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

//
// These standard headers are required for all FX-RTOS distributions.
//
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include FX_INTERFACE(CFG_OPTIONS)

#define FX_STATUS_OK 0

//!
//! Error checking is disabled.
//!
#if (!defined LANG_ASSERT_ERROR_CHECKING_TYPE) || (LANG_ASSERT_ERROR_CHECKING_TYPE == 0)
#define lang_param_assert(assert, errcode) 

//!
//! Classic error checking policy.
//!
#elif (LANG_ASSERT_ERROR_CHECKING_TYPE == 1)
#define lang_param_assert(assert, err_code) if (!(assert)) {return (err_code);}

//!
//! Centralized error checking policy: user function call on error.
//!
#elif (LANG_ASSERT_ERROR_CHECKING_TYPE == 2)

//!
//! User-supplied function for error handling.
//!
void fx_error_catch(const char* func, const char* err_code_str, int err_code);

//!
//! Helper macro to get error code as a string. 
//!
#define lang_param_assert_return_str(cond, err_code_str, err_code) \
    if (!(cond)) { fx_error_catch(__func__, err_code_str, err_code); }

//!
//! Parameter checking. 
//! @param assert Condition.
//! @param err_code Unique identifier of assert in module (i.e. err code).
//! @warning This macro should be used inside functions at top level only.
//!
#define lang_param_assert(assert, err_code) \
    lang_param_assert_return_str(assert, #err_code, err_code)

#else
#error Unknown error checking type!
#endif

#define lang_type_to_bits(type) (sizeof(type) * 8)
#define lang_bits_to_words(n) \
    (((n) + lang_type_to_bits(unsigned) - 1)/(lang_type_to_bits(unsigned)))

#define lang_containing_record(address, type, field) \
    ((type*)((char*)(address) - (size_t)(&((type*)0)->field)))

#if __STDC_VERSION__ > 201000L
#define lang_static_assert(cond) _Static_assert(cond, #cond)
#else
#define __lang_static_assert(cond, line) \
    typedef int static_assertion_##line[((!(!(cond))) * 2) - 1]
#define ___lang_static_assert(cond, line) __lang_static_assert(cond, line)
#define lang_static_assert(cond) ___lang_static_assert(cond, __LINE__)
#endif

#define lang_min(a, b) ((a) < (b) ? (a) : (b))
#define lang_max(a, b) ((a) > (b) ? (a) : (b))

FX_METADATA(({ interface: [LANG_TYPES, OS_186] }))

FX_METADATA(({ options: [                                               
    LANG_ASSERT_ERROR_CHECKING_TYPE: {                                            
        type: enum, values: [Off: 0, Classic: 1, Centralized: 2], default: 0,  
        description: "Default error checking policy." }]}))

#endif 
