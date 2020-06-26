#ifndef _FX_SCHED_ALG_MPQ_FIFO_HEADER_
#define _FX_SCHED_ALG_MPQ_FIFO_HEADER_

/** 
  ******************************************************************************
  *  @file   mpq/fx_sched_alg.h
  *  @brief  Interface of scheduler container.
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

#include FX_INTERFACE(CFG_OPTIONS)
#include FX_INTERFACE(RTL_QUEUE)
#include FX_INTERFACE(LANG_TYPES)

#ifndef FX_SCHED_ALG_PRIO_NUM
#define FX_SCHED_ALG_PRIO_NUM 64
#endif

#if FX_SCHED_ALG_PRIO_NUM > 1024
#error Too many priority levels, it must be less than 1024!
#endif

//! 
//! Scheduler container representation. 
//! It contains N queues for each priority and bitmaps for fast search.
//!
typedef struct
{
    rtl_queue_t priority_queues[FX_SCHED_ALG_PRIO_NUM];
    unsigned map1;
    unsigned map2[lang_bits_to_words(FX_SCHED_ALG_PRIO_NUM)];
}
fx_sched_container_t;

//!
//! Scheduler container's item. 
//!
typedef struct
{
    unsigned int prio;
    rtl_queue_linkage_t link;
} 
fx_sched_params_t;

//!
//! Lowest priority has maximum numeric value starting from 0.
//!
#define FX_SCHED_ALG_PRIO_IDLE (FX_SCHED_ALG_PRIO_NUM - 1)

//!
//! Cast scheduling params as integer. 
//! It is also used for "priority visualization" by external tools.
//!
#define fx_sched_params_as_number(s) ((s)->prio)

//!
//! Checks whether sched params A preempts parameters B.
//!
#define fx_sched_params_is_preempt(a, b) ((a)->prio < (b)->prio)

//!
//! Check for params equality.
//!
#define fx_sched_params_is_equal(a, b) ((a)->prio == (b)->prio)

//!
//! Checks whether the item unique at given priority level. 
//! It means that no pending work exist.
//!
#define fx_sched_params_is_unique(a) \
    (rtl_queue_first(&((a)->link)) == rtl_queue_last(&((a)->link)))

//!
//! Copy constructor.
//! @param src Source item from which params will be copied to destination item.
//! @param dst Destination item.
//!
#define fx_sched_params_copy(src, dst) ((dst)->prio = (src)->prio)

//!
//! Schedulable item initializer. (priority-specific method).
//! @param [in] item Schedulable item to be initialized.
//! @param [in] priority Priority which to be set in schedulable item.
//!
#define fx_sched_params_init_prio(item, priority) ((item)->prio = (priority))

//! 
//! Default initializers for scheduler container's items. 
//!
typedef enum
{
    FX_SCHED_PARAMS_INIT_IDLE = 0,  //!< This value is used to init IDLE-entity.
    FX_SCHED_PARAMS_INIT_DEFAULT,   //!< Default priority.
    FX_SCHED_PARAMS_INIT_SPECIFIED  //!< Copy sched params from another item.
}
fx_sched_params_init_t;

void fx_sched_params_init(
    fx_sched_params_t* params, 
    fx_sched_params_init_t type, 
    const fx_sched_params_t* src
);

void fx_sched_container_init(fx_sched_container_t* c);
void fx_sched_container_add(fx_sched_container_t* c, fx_sched_params_t* p);
void fx_sched_container_remove(fx_sched_container_t* c, fx_sched_params_t* p);
fx_sched_params_t* fx_sched_container_get(fx_sched_container_t* c);

FX_METADATA(({ interface: [FX_SCHED_ALG, MPQ_FIFO] }))

FX_METADATA(({ options: [                                               
    FX_SCHED_ALG_PRIO_NUM: {                                                    
        type: int, range: [8, 1024], default: 64,                               
        description: "Number of scheduling priorities."}]}))

#endif
