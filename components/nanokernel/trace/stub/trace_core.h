#ifndef _TRACE_THREAD_STUB_HEADER_
#define _TRACE_THREAD_STUB_HEADER_

/** 
  ******************************************************************************
  *  @file   trace_core.h
  *  @brief  Stub for trace subsystem. Disables thread tracing.
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

typedef struct {int dummy;} trace_thread_handle_t;
typedef struct {int dummy;} trace_queue_handle_t;
typedef struct {int dummy;} trace_sem_handle_t;
typedef struct {int dummy;} trace_mutex_handle_t;

#define trace_increment_tick( incremented_counter )

#define trace_mutex_init( mutex_handle )  
#define trace_mutex_init_failed() 
#define trace_mutex_deinit( mutex_handle )  
#define trace_mutex_acquired( mutex_handle , owner_thread_handle)
#define trace_mutex_acquire_block( mutex_handle )
#define trace_mutex_released( mutex_handle, new_owner_handle )

#define trace_sem_init( sem_handle, sem_val, sem_max )
#define trace_sem_init_failed()
#define trace_sem_deinit( sem_handle, sem_val, sem_max )
#define trace_sem_wait_ok( sem_handle, sem_val )
#define trace_sem_wait_block( sem_handle, blocked_thread_handle )
#define trace_sem_post( sem_handle, sem_val )

#define trace_queue_init( queue_handle, items_max )
#define trace_queue_init_failed( queue_handle )
#define trace_queue_deinit( queue_handle, msg_count )
#define trace_queue_send( queue_handle, msg_count )
#define trace_queue_send_failed( queue_handle )
#define trace_queue_send_block( queue_handle )
#define trace_queue_send_forward( queue_handle)
#define trace_queue_receive( queue_handle, msg_count )
#define trace_queue_receive_failed( queue_handle )
#define trace_queue_receive_block( queue_handle )
#define trace_queue_receive_forward( queue_handle )

#define trace_thread_init_idle(thread_handle, prio)
#define trace_thread_init( thread_handle, prio )
#define trace_thread_init_failed()
#define trace_thread_deinit( thread_handle, prio )
#define trace_thread_suspend( thread_handle )
#define trace_thread_resume( thread_handle )
#define trace_thread_wakeup( thread_handle )
#define trace_thread_context_switch(from, to)
#define trace_thread_sleep(thread_handle, ticks)
#define trace_thread_delay_until(thread_handle, ticks) 
#define trace_thread_sched_param_set( thread_handle, param )
#define trace_thread_ceiling( thread_handle, old_prio, new_prio )
#define trace_thread_deceiling( thread_handle, old_prio, new_prio )
#define trace_thread_timeout( thread_handle, timeout )

FX_METADATA(({ interface: [TRACE_CORE, STUB] }))

#endif
