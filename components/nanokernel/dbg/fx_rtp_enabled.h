#ifndef _FX_RTP_ENABLED_HEADER_
#define _FX_RTP_ENABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_rtp_enabled.h
  *  @brief  Interface to run-time protection module (canary like).
  *  Runtime protection is implemented as 'magic-values'.
  *  Every object has special internal member and unique key value.
  *  Every time when someone is going to use any object it examines that member
  *  which should be equal to magic value.
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

typedef uint32_t fx_rtp_t, fx_rtp_part_t, fx_rtp_key_t;

#define fx_rtp_init(target, key) ((*(target)) = (key))
#define fx_rtp_deinit(target) ((*(target)) = 0)
#define fx_rtp_check(target, key) ((*(target)) == (key))

//
// Partial object protection does not need a destructor.
//
#define fx_rtp_part_init(target, key) fx_rtp_init(target, key)
#define fx_rtp_part_check(target, key) fx_rtp_check(target, key)

FX_METADATA(({ interface: [FX_RTP, ENABLED] }))

#endif
