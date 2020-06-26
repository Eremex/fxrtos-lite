#ifndef _FX_SCHED_ALG_BITMAP_HEADER_
#define _FX_SCHED_ALG_BITMAP_HEADER_

/** 
  ******************************************************************************
  *  @file   bitmap/fx_sched_alg.h
  *  @brief  Implementation of bitmap-based scheduler container.
  *  Scheduler container is designed as pair of bitmap and array, where each bit
  *  of the array indicates that corresponding array member is non-null.
  *  N.B. There's only 32 priority levels available and only one item with given
  *  priority is allowed.
  *  N.B. All container functions expect SPL = SCHED_LEVEL.
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
#include FX_INTERFACE(HW_CPU)

//! 
//! Priority definitions.
//!
typedef uint_fast8_t _fx_sched_params_t, fx_sched_params_t;

//! 
//! Idle priority is defined as most significant bit number of default unsigned 
//! type. It is 31 on 32-bit platforms, 15 on 16-bit ones and so on.
//!
#define FX_SCHED_ALG_PRIO_IDLE (lang_type_to_bits(unsigned int) - 1)

//!
//! Scheduling params as number. It is used for "priority visualization" by some
//! external tools.
//!
#define fx_sched_params_as_number(s)  (*(s))

//!
//! Checks whether sched params A preempts parameters B.
//!
#define fx_sched_params_is_preempt(a, b)  (*(a) < *(b))

//!
//! Check for params equality.
//!
#define fx_sched_params_is_equal(a, b)  (*(a) == *(b))

//!
//! Checks whether the item unique at given priority level. It means that no 
//! pending work exist. This container implements only one item per priority, 
//! so, any item is always unique.
//!
#define fx_sched_params_is_unique(a)  true

//!
//! Copying of one schedulable item to another.
//! Only params are copied, not item-specific stuff like queue links.
//! @param src Source item from which params will be copied to destination item.
//! @param dst Destination item.
//!
#define fx_sched_params_copy(src, dst) (*(dst) = *(src))

//!
//! Schedulable item initializer.
//! Initialize schedulable item by specified priority.
//! @param [in] item Schedulable item to be initialized.
//! @param [in] priority Priority which to be set in schedulable item.
//!
#define fx_sched_params_init_prio(item, priority) (*(item) = (priority))

//! 
//! Default initializers for scheduler container's items. 
//!
typedef enum
{
    FX_SCHED_PARAMS_INIT_IDLE = 0,  //!< The value is used to create IDLE-entity
    FX_SCHED_PARAMS_INIT_DEFAULT,   //!< Default priority.
    FX_SCHED_PARAMS_INIT_SPECIFIED  //!< Copy sched params from another item.
}
fx_sched_params_init_t;

//!
//! Constructor of schedulable item.
//! @param [in,out] item Target schedulable item to be initialized.
//! @param [in] type Initialization type.
//! @param [in] src  Schedulable item that will be used as source of params in 
//! case when FX_SCHED_PARAMS_INIT_SPECIFIED specified as the type.
//! @remark src parameter cannot be NULL if type is 
//! FX_SCHED_PARAMS_INIT_SPECIFIED.
//!
static inline void 
fx_sched_params_init(
    fx_sched_params_t* item, 
    fx_sched_params_init_t type, 
    const fx_sched_params_t* src)
{
    switch (type)
    {
    case FX_SCHED_PARAMS_INIT_IDLE: *item = FX_SCHED_ALG_PRIO_IDLE; break;
    case FX_SCHED_PARAMS_INIT_DEFAULT: *item = FX_SCHED_ALG_PRIO_IDLE-1; break;
    case FX_SCHED_PARAMS_INIT_SPECIFIED: *item = *src; break;
    };
}

//! 
//! Scheduler container representation. 
//! For this scheduler it contains N slots for each priority.
//!
typedef struct
{
    unsigned int items_map;
    fx_sched_params_t* items[lang_type_to_bits(unsigned int)];
}
fx_sched_container_t;

//!
//! Constructor of the scheduler container. It is expected that items array is 
//! filled by zeros.
//! @param [in] container Scheduler container.
//!
#define fx_sched_container_init(container) (container)->items_map = 0

//!
//! Add to container.
//! This function adds specified schedulable item into container. Item must be 
//! properly initialized.
//! @param [in] c Target scheduler container.
//! @param [in] item Schedulable item to be added into the container.
//!
#define fx_sched_container_add(c, i) \
    ((c)->items[*(i)] = (i), (c)->items_map |= (1 << *(i)))

//!
//! Remove from container.
//! This function removes specified schedulable item from container.
//! @param [in] c Target scheduler container.
//! @param [in] item Schedulable item to be added into the container.
//!
#define fx_sched_container_remove(c, item) \
    (c)->items[*(item)] = NULL, (c)->items_map &= ~(1 << *(item))

//!
//! Schedule function.
//! It selects schedulable item with highest priority from items which are 
//! contained in the specified container.
//! @param [in] container Container where scheduing should be performed.
//! @return Scheduled item. 
//! @warning  Container must contain at least one element before this function 
//! call.
//!
#define fx_sched_container_get(container) \
    (container)->items[hw_cpu_ctz((container)->items_map)]  

FX_METADATA(({ interface: [FX_SCHED_ALG, BITMAP] }))

#endif
