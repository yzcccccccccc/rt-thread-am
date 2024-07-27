#include "rtdef.h"
#include <am.h>
#include <klib.h>
#include <rtthread.h>
#include <stdint.h>
#include <sys/types.h>

static Context* ev_handler(Event e, Context *c) {
  rt_ubase_t *data = (rt_ubase_t *)rt_thread_self()->user_data;

  rt_ubase_t from = data[0];
  rt_ubase_t to = data[1];

  if (from != 0){
    *(Context **)from = c;
  }

  Context *target_ctx = *(Context **)to;
  switch (e.event) {
    case EVENT_YIELD:
      return target_ctx;
      break;
    case EVENT_IRQ_TIMER:
      return c;
      break;
    default: printf("Unhandled event ID = %d\n", e.event); assert(0);
  }
  return c;
}

void __am_cte_init() {
  cte_init(ev_handler);
}

void rt_hw_context_switch_to(rt_ubase_t to) {
  rt_ubase_t tmp = rt_thread_self()->user_data;
  rt_ubase_t data[2];   // from | to
  data[0] = 0;
  data[1] = to;
  rt_thread_self()->user_data = (rt_ubase_t)data;
  yield();
  rt_thread_self()->user_data = tmp;
  //assert(0);
}

void rt_hw_context_switch(rt_ubase_t from, rt_ubase_t to) {
  rt_ubase_t tmp = rt_thread_self()->user_data;
  rt_ubase_t data[2];   // from | to
  data[0] = from;
  data[1] = to;
  rt_thread_self()->user_data = (rt_ubase_t)data;
  yield();
  rt_thread_self()->user_data = tmp;
  //assert(0);
}

void rt_hw_context_switch_interrupt(void *context, rt_ubase_t from, rt_ubase_t to, struct rt_thread *to_thread) {
  assert(0);
}

#define ROUNDUP(a, sz)   ((((uintptr_t)a) + (sz) - 1) & ~((sz) - 1))
#define ROUNDDOWN(a, sz) ((((uintptr_t)a)) & ~((sz) - 1))

typedef void (* EXIT_PTR)();
typedef void (* ENTRY_PTR)(void *);

void entry_wrap(void *stptr){
  // l: texit | parameter | tentry :h
  uintptr_t exit_ptr  = *(uintptr_t *)stptr;
  uintptr_t parameter = *(uintptr_t *)(stptr + sizeof(uintptr_t));
  uintptr_t entry_ptr = *(uintptr_t *)(stptr + 2 * sizeof(uintptr_t));

  ((ENTRY_PTR)entry_ptr)((void *)parameter);
  ((EXIT_PTR)exit_ptr)();
} 

rt_uint8_t *rt_hw_stack_init(void *tentry, void *parameter, rt_uint8_t *stack_addr, void *texit) {
  uintptr_t stack_base = ROUNDDOWN(stack_addr, sizeof(uintptr_t));
  uintptr_t stack_end = stack_base - 3 * sizeof(uintptr_t);
  uintptr_t *sp = (uintptr_t *)stack_end;
  sp[0] = (uintptr_t)texit;
  sp[1] = (uintptr_t)parameter;
  sp[2] = (uintptr_t)tentry;
  Context *ptr = kcontext((Area){(void *)NULL/* TODO don't care?*/, (void *)(stack_end)}, entry_wrap, (void *)sp);
  //assert(0);
  return (rt_uint8_t *)ptr;
}