#include "violin.h"

#ifndef SCAL_BENCHMARK_STD_PIPE_API_H_
#define SCAL_BENCHMARK_STD_PIPE_API_H_

#include <stdint.h>

extern uint64_t g_num_threads;

extern void* ds_new(void);
extern bool ds_put(void *ds, uint64_t val);
extern bool ds_get(void *ds, uint64_t *val);
extern char* ds_get_stats(void);

#endif  // SCAL_BENCHMARK_STD_PIPE_API_H_

#include "datastructures/balancer_partrr.h"
#include "datastructures/boundedsize_kfifo.h"
#include "datastructures/distributed_queue.h"
#include "datastructures/dts_queue.h"
#include "datastructures/flatcombining_queue.h"
#include "datastructures/kstack.h"
#include "datastructures/lockbased_queue.h"
#include "datastructures/ms_queue.h"
#include "datastructures/random_dequeue_queue.h"
#include "datastructures/single_list.h"
#include "datastructures/treiber_stack.h"
#include "datastructures/ts_deque.h"
#include "datastructures/ts_deque_buffer.h"
#include "datastructures/ts_queue.h"
#include "datastructures/ts_queue_buffer.h"
#include "datastructures/ts_stack.h"
#include "datastructures/ts_stack_buffer.h"
#include "datastructures/ts_timestamp.h"
#include "datastructures/unboundedsize_kfifo.h"
#include "datastructures/wf_queue_ppopp11.h"
// #include "datastructures/wf_queue_ppopp12.h"

Pool<int> *obj;
enum { BK_QUEUE, D_QUEUE, DTS_QUEUE, FC_QUEUE, LB_QUEUE, MS_QUEUE, K_STACK, RD_QUEUE, S_LIST, TR_STACK, TS_DEQUE, TS_QUEUE, TS_STACK, UK_QUEUE, WF_QUEUE_11, WF_QUEUE_12 };
int violin_object;

int obj_order() {
  switch (violin_object) {
  case BK_QUEUE:
  case D_QUEUE:
  case DTS_QUEUE:
  case FC_QUEUE:
  case LB_QUEUE:
  case MS_QUEUE:
  case RD_QUEUE:
  case S_LIST:
  case TS_QUEUE:
  case UK_QUEUE:
  case WF_QUEUE_11:
  case WF_QUEUE_12:
    return FIFO_ORDER;
  case K_STACK:
  case TR_STACK:
  case TS_STACK:
    return LIFO_ORDER;
  case TS_DEQUE:
  default:
    return NO_ORDER;
  }
}

int k = 2;
int num_segments = 2;
int num_queues = 2;
int partitions = 2;
int dequeue_mode = 0;
int dequeue_timeout = 0;
int num_ops = 2;
int quasi_factor = 2;
int max_retries = 2;
int delay = 2;
int helping_delay = 2;

Pool<int>* obj_create() {
  switch (violin_object) {
  case BK_QUEUE: return new BoundedSizeKFifo<int>(k, num_segments);
  case D_QUEUE: return new DistributedQueue< int, MSQueue<int> >(num_queues,g_num_threads+1,new BalancerPartitionedRoundRobin(partitions,num_queues));
  case DTS_QUEUE: return (Pool<int>*) new DTSQueue<int>();
  case LB_QUEUE: return (Pool<int>*) new LockBasedQueue<int>(dequeue_mode,dequeue_timeout);
  case MS_QUEUE: return new MSQueue<int>();
  case FC_QUEUE: return new FlatCombiningQueue<int>(num_ops);
  case K_STACK: return new KStack<int>(k,g_num_threads+1);
  case RD_QUEUE: return new RandomDequeueQueue<int>(quasi_factor, max_retries);
  case S_LIST: return new SingleList<int>();
  case TR_STACK: return new TreiberStack<int>();
  case TS_DEQUE: return new TSDeque<int,TSDequeBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  case TS_QUEUE: return new TSQueue<int,TSQueueBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  case TS_STACK: return new TSStack<int,TSStackBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  case UK_QUEUE: return new UnboundedSizeKFifo<int>(k);
  case WF_QUEUE_11: return new WaitfreeQueue<int>(g_num_threads+1);
  // case WF_QUEUE_12: return new WaitfreeQueue<int>(g_num_threads+1, max_retries, helping_delay);
  default:
    assert(false);
  }
}

void obj_reset() {
  if (obj) delete obj;
  obj = obj_create();
}
void obj_add(int v) {
  obj->put(v);
}
int obj_rem() {
  int result;
  obj->get(&result);
  return result;
}

uint64_t g_num_threads;
int main() {
  // google::SetUsageMessage(usage);
  // google::ParseCommandLineFlags(&argc, const_cast<char***>(&argv), true);
  // uint64_t tlsize = scal::human_size_to_pages(FLAGS_prealloc_size.c_str(),
  //                                             FLAGS_prealloc_size.size());
  uint64_t tlsize = 4;
  // Init the main program as executing thread (may use rnd generator or tl
  // allocs).
  // g_num_threads = FLAGS_producers + FLAGS_consumers;
  g_num_threads = 4;
  scal::tlalloc_init(tlsize, true /* touch pages */);
  //threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

  // violin_object = BK_QUEUE;
  // violin_object = D_QUEUE;
  // violin_object = DTS_QUEUE; // X -- segfault
  // violin_object = LB_QUEUE;
  // violin_object = MS_QUEUE;
  // violin_object = FC_QUEUE;
  // violin_object = K_STACK; // X -- mod after free (?)
  // violin_object = RD_QUEUE;
  // violin_object = S_LIST; // NOTE: he's returning 0 for EMPTY!
  // violin_object = TR_STACK;
  // violin_object = TS_DEQUE; // X -- mod after free (?)
  // violin_object = TS_QUEUE; // X -- mod after free (?)
  // violin_object = TS_STACK; // X -- mod after free (?)
  // violin_object = UK_QUEUE;
  violin_object = WF_QUEUE_11;
  // violin_object = WF_QUEUE_12;

  violin(obj_reset,obj_add,1,obj_rem,2,DEFAULT_ALLOC,obj_order(),0,1);
  return 0;
}