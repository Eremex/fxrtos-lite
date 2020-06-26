#ifndef _FX_MSGQ_ESR_V1_HEADER_
#define _FX_MSGQ_ESR_V1_HEADER_

/** 
  ******************************************************************************
  *  @file   fx_msgq_esr.h
  *  @brief  Interface of message queue services for ESRs.
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

#include FX_INTERFACE(FX_MSGQ_CORE)
#include FX_INTERFACE(FX_ESR)

enum
{
    FX_MSGQ_OK = FX_STATUS_OK,
    FX_MSGQ_INVALID_PTR,
    FX_MSGQ_INVALID_BUF,
    FX_MSGQ_INVALID_OBJ,
    FX_MSGQ_FULL,
    FX_MSGQ_NO_MSG,
    FX_MSGQ_UNSUPPORTED_POLICY,
    FX_MSGQ_ERR_MAX
};

int fx_msgq_init(
  fx_msgq_t* msgq, 
  uintptr_t* buf, 
  unsigned int sz, 
  fx_sync_policy_t p
);
int fx_msgq_deinit(fx_msgq_t* msgq);
int fx_msgq_flush(fx_msgq_t* msgq);
int fx_msgq_send(fx_msgq_t* msgq, uintptr_t msg);
int fx_msgq_listen(fx_msgq_t* msgq, const bool wait);

FX_METADATA(({ interface: [FX_MSGQ_ESR, V1] }))

#endif
