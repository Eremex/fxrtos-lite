#ifndef _FX_THREAD_CLEANUP_DISABLED_HEADER_
#define _FX_THREAD_CLEANUP_DISABLED_HEADER_

/** 
  ******************************************************************************
  *  @file   disabled/fx_thread_cleanup.h
  *  @brief  Disables functionality of in-thread cleanup handlers.
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

typedef struct { int dummy; } fx_thread_cleanup_context_t;
typedef struct { int dummy; } fx_thread_cleanup_handler_t;

#define fx_thread_cleanup_switch_hook(old_target)
#define fx_thread_cleanup_init_target(target)
#define fx_thread_cleanup_handle(target)
#define fx_thread_cleanup_init(handler, f, a) ((void) (handler))
#define fx_thread_cleanup_add(target, k, handler)
#define fx_thread_cleanup_cancel(handler)
#define fx_thread_cleanup_set_hook(target, func)

FX_METADATA(({ interface: [FX_THREAD_CLEANUP, DISABLED] }))

#endif 
