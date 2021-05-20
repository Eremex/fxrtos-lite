#ifndef _RTL_LIST_V1_HEADER_
#define _RTL_LIST_V1_HEADER_

/*
  * Copyright (c) 2005-2007, Kohsuke Ohtani
  * All rights reserved.
  *
  * Redistribution and use in source and binary forms, with or without
  * modification, are permitted provided that the following conditions
  * are met:
  * 1. Redistributions of source code must retain the above copyright
  *    notice, this list of conditions and the following disclaimer.
  * 2. Redistributions in binary form must reproduce the above copyright
  *    notice, this list of conditions and the following disclaimer in the
  *    documentation and/or other materials provided with the distribution.
  * 3. Neither the name of the author nor the names of any co-contributors
  *    may be used to endorse or promote products derived from this software
  *    without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
  * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
  * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
  * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
  * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
  * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
  * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
  * SUCH DAMAGE.
  */

#include <stddef.h>

typedef struct _rtl_list_t 
{
    struct _rtl_list_t  *next;
    struct _rtl_list_t  *prev;
}
rtl_list_t, rtl_list_linkage_t;

#define rtl_list_init(head) ((head)->next = (head)->prev = (head))
#define rtl_list_next(node) ((node)->next)
#define rtl_list_prev(node) ((node)->prev)
#define rtl_list_empty(head) ((head)->next == (head))
#define rtl_list_first(head) ((head)->next)
#define rtl_list_last(head) ((head)->prev)
#define rtl_list_end(head, node) ((node) == (head))
#define rtl_list_is_node_linked(node) (((node)->next) && ((node)->prev))
#define rtl_list_entry(p, type, member) \
    ((type*)((char*)(p) - (size_t)(&((type*)0)->member)))

static inline void
rtl_list_insert(rtl_list_t* prev, rtl_list_t* node)
{
    node->next = prev->next;
    node->prev = prev;
    prev->next->prev = node;
    prev->next = node;
}

static inline void
rtl_list_remove(rtl_list_t* node)
{
    node->prev->next = node->next;
    node->next->prev = node->prev;
    node->next = (rtl_list_linkage_t*)0;
    node->prev = (rtl_list_linkage_t*)0;
}

static inline void 
rtl_list_insert_range(rtl_list_t* dst, rtl_list_t* src)
{
    dst->prev->next = src->next;
    src->next->prev = dst->prev;
    dst->prev = src->prev;
    src->prev->next = dst;
}

FX_METADATA(({ interface: [RTL_LIST, V1] }))

#endif
