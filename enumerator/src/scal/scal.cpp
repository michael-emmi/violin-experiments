#include <gflags/gflags.h>
#include <sstream>

#include "violin.h"

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

extern uint64_t g_num_threads;
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

Pool<int> *obj;
int violin_object;

enum {
  BK_QUEUE, D_QUEUE, DTS_QUEUE, FC_QUEUE, LB_QUEUE, MS_QUEUE, K_STACK,
  RD_QUEUE, S_LIST, TR_STACK, TS_DEQUE, TS_QUEUE, TS_STACK, UK_QUEUE,
  WF_QUEUE_11, WF_QUEUE_12
};

struct obj_desc {
  int id;
  int order;
  string short_name;
  string long_name;
};

obj_desc objects[] = {
  { .id = BK_QUEUE, .order = FIFO_ORDER, .short_name = "bkq", .long_name = "Bounded-size K FIFO" },
  { .id = D_QUEUE, .order = FIFO_ORDER, .short_name = "dq", .long_name = "Distributed Queue" },
  { .id = DTS_QUEUE, .order = FIFO_ORDER, .short_name = "dtsq", .long_name = "DTS Queue" },
  { .id = FC_QUEUE, .order = FIFO_ORDER, .short_name = "fcq", .long_name = "FC Queue" },
  { .id = K_STACK, .order = LIFO_ORDER, .short_name = "ks", .long_name = "K Stack" },
  { .id = LB_QUEUE, .order = FIFO_ORDER, .short_name = "lbq", .long_name = "Lock-based Queue" },
  { .id = MS_QUEUE, .order = FIFO_ORDER, .short_name = "msq", .long_name = "MS Queue" },
  { .id = RD_QUEUE, .order = FIFO_ORDER, .short_name = "rdq", .long_name = "Random-dequeue Queue" },
  { .id = S_LIST, .order = FIFO_ORDER, .short_name = "sl", .long_name = "Single List" },
  { .id = TR_STACK, .order = LIFO_ORDER, .short_name = "ts", .long_name = "Treiber Stack" },
  { .id = TS_DEQUE, .order = NO_ORDER, .short_name = "tsd", .long_name = "TS Deque" },
  { .id = TS_STACK, .order = LIFO_ORDER, .short_name = "tss", .long_name = "TS Stack" },
  { .id = TS_QUEUE, .order = FIFO_ORDER, .short_name = "tsq", .long_name = "TS Queue" },
  { .id = UK_QUEUE, .order = FIFO_ORDER, .short_name = "ukq", .long_name = "Unbounded-size K FIFO" },
  { .id = WF_QUEUE_11, .order = FIFO_ORDER, .short_name = "wfq11", .long_name = "Wait-free Queue (2011)" },
  { .id = WF_QUEUE_12, .order = FIFO_ORDER, .short_name = "wfq12", .long_name = "Wait-free Queue (2012)" },
};

Pool<int>* obj_create(int id) {
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

int obj_id(string name) {
  int len = sizeof(objects) / sizeof(obj_desc);
  for (int i=0; i<len; i++)
    if (objects[i].short_name == name) {
      return objects[i].id;
    }
  return -1;
}

string obj_name(int id) {
  int len = sizeof(objects) / sizeof(obj_desc);
  for (int i=0; i<len; i++)
    if (objects[i].id == id)
      return objects[i].long_name;
  return "????";
}

int obj_order(int id) {
  int len = sizeof(objects) / sizeof(obj_desc);
  for (int i=0; i<len; i++)
    if (objects[i].id == id)
      return objects[i].order;
  return NO_ORDER;
}

void obj_reset() {
  if (obj) delete obj;
  obj = obj_create(violin_object);
}

void obj_add(int v) {
  obj->put(v);
}

int obj_rem() {
  int result;
  if (obj->get(&result))
    return result;
  else
    return -1;
}

DEFINE_int32(adds, 1, "how many add operations?");
DEFINE_int32(removes, 1, "how many remove operations?");
DEFINE_int32(barriers, 0, "how many barriers?");
DEFINE_int32(delays, 0, "how many delays?");
DEFINE_int32(alloc, 0, "allocation policy? 0=default, 1=LRF, 2=MRF");

uint64_t g_num_threads;

int main(int argc, char **argv) {

  stringstream usage;
  usage << "usage" << endl;
  usage << "  " << argv[0] << " [flags] OBJ, where OBJ comes from:" << endl;
  int len = sizeof(objects) / sizeof(obj_desc);
  for (int i=0; i<len; i++) {
    usage << "    " << objects[i].short_name << " -> " << objects[i].long_name << endl;
  }
  usage << "  then see the flags below.";

  google::SetUsageMessage(usage.str());
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (argc-1 != 1) {
    cerr << "Must specify one data structure; see --help for usage." << endl;
    exit(-1);
  }

  if ((violin_object = obj_id(argv[1])) < 0) {
    cerr << "Invalid data structure name \"" << argv[1] << "\"; see --help for usage." << endl;
    exit(-1);
  }

  // TODO are we initializing these correctly?
  // TODO investigate!
  uint64_t tlsize = 4;
  g_num_threads = 4;
  scal::tlalloc_init(tlsize, true /* touch pages */);
  //threadlocals_init();
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();

  cout << "Running SCAL data structure: " << obj_name(violin_object) << endl;
  violin(
    obj_reset,
    obj_add, FLAGS_adds,
    obj_rem, FLAGS_removes,
    FLAGS_alloc,
    obj_order(violin_object),
    FLAGS_barriers,
    FLAGS_delays
  );
  return 0;
}