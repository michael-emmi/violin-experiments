#include <gflags/gflags.h>
#include <sstream>
#include <map>
#include <iostream>

#include "scal.h"

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

const string DEFAULT_PAGE_SIZE = "1k";
const unsigned DEFAULT_K = 10;
const unsigned DEFAULT_NUM_SEGMENTS = 1000;
const unsigned DEFAULT_NUM_QUEUES = 2;
const unsigned DEFAULT_PARTITIONS = 2;
const unsigned DEFAULT_DEQUEUE_MODE = 0; // from 0,1,2
const unsigned DEFAULT_DEQUEUE_TIMEOUT = 0;
const unsigned DEFAULT_NUM_OPS = 2;
const unsigned DEFAULT_QUASI_FACTOR = 2;
const unsigned DEFAULT_MAX_RETRIES = 2;
const unsigned DEFAULT_DELAY = 2;
const unsigned DEFAULT_HELPING_DELAY = 2;

extern uint64_t g_num_threads;
uint64_t g_num_threads;
unsigned k;
unsigned num_segments;
unsigned num_queues;
unsigned partitions;
unsigned dequeue_mode;
unsigned dequeue_timeout;
unsigned num_ops;
unsigned quasi_factor;
unsigned max_retries;
unsigned delay;
unsigned helping_delay;

map<string,obj_desc> objects;

vector<bool> thread_initialized;

void ensure_thread_initialized() {
  uint64_t id = scal::ThreadContext::get().thread_id();
  while (thread_initialized.size() <= id)
    thread_initialized.push_back(false);
  if (!thread_initialized[id]) {
    uint64_t tlsize = scal::human_size_to_pages(
      DEFAULT_PAGE_SIZE.c_str(),DEFAULT_PAGE_SIZE.size());
    scal::tlalloc_init(tlsize, true /* touch pages */);
  }
}

#define DECLARE_OBJ(ID,NAME,SPEC) \
  objects[ID] = { .id = ID, .name = NAME, .spec = SPEC }

void scal_initialize(unsigned num_threads) {
  DECLARE_OBJ("bkq",    "Bounded-size-K-FIFO",    "atomic-queue");
  DECLARE_OBJ("dq",     "Distributed-Queue",      "atomic-queue");
  DECLARE_OBJ("dtsq",   "DTS-Queue",              "atomic-queue");
  DECLARE_OBJ("fcq",    "Flat-combining-Queue",   "atomic-queue");
  DECLARE_OBJ("ks",     "K-Stack",                "atomic-stack");
  DECLARE_OBJ("lbq",    "Lock-based-Queue",       "atomic-queue");
  DECLARE_OBJ("msq",    "MS-Queue",               "atomic-queue");
  DECLARE_OBJ("rdq",    "Random-dequeue-Queue",   "atomic-queue");
  DECLARE_OBJ("sl",     "Single-List",            "atomic-queue");
  DECLARE_OBJ("ts",     "Treiber-Stack",          "atomic-stack");
  DECLARE_OBJ("tsd",    "TS-Deque",               "atomic-collection");
  DECLARE_OBJ("tss",    "TS-Stack",               "atomic-stack");
  DECLARE_OBJ("tsq",    "TS-Queue",               "atomic-queue");
  DECLARE_OBJ("ukq",    "Unbounded-size-K-FIFO",  "atomic-queue");
  DECLARE_OBJ("wfq11",  "Wait-free-Queue-2011", "atomic-queue");
  DECLARE_OBJ("wfq12",  "Wait-free-Queue-2012", "atomic-queue");

	k = DEFAULT_K;
	num_segments = DEFAULT_NUM_SEGMENTS;
	num_queues = DEFAULT_NUM_QUEUES;
	partitions = DEFAULT_PARTITIONS;
	dequeue_mode = DEFAULT_DEQUEUE_MODE;
	dequeue_timeout = DEFAULT_DEQUEUE_TIMEOUT;
	num_ops = DEFAULT_NUM_OPS;
	quasi_factor = DEFAULT_QUASI_FACTOR;
	max_retries = DEFAULT_MAX_RETRIES;
	delay = DEFAULT_DELAY;
	helping_delay = DEFAULT_HELPING_DELAY;

	g_num_threads = num_threads;
  uint64_t tlsize = scal::human_size_to_pages(
    DEFAULT_PAGE_SIZE.c_str(),DEFAULT_PAGE_SIZE.size());
  scal::tlalloc_init(tlsize, true /* touch pages */);
  scal::ThreadContext::prepare(g_num_threads + 1);
  scal::ThreadContext::assign_context();
}

Pool<int>* obj_create(string obj) {
  if (obj == "bkq")
    return new BoundedSizeKFifo<int>(k, num_segments);
  else if (obj == "dq")
    return new DistributedQueue< int, MSQueue<int> >(num_queues,g_num_threads+1,new BalancerPartitionedRoundRobin(partitions,num_queues));
  else if (obj == "dtsq")
    return (Pool<int>*) new DTSQueue<int>();
  else if (obj == "lbq")
    return (Pool<int>*) new LockBasedQueue<int>(dequeue_mode,dequeue_timeout);
  else if (obj == "msq")
    return new MSQueue<int>();
  else if (obj == "fcq")
    return new FlatCombiningQueue<int>(num_ops);
  else if (obj == "ks")
    return new KStack<int>(k,g_num_threads+1);
  else if (obj == "rdq")
    return new RandomDequeueQueue<int>(quasi_factor, max_retries);
  else if (obj == "sl")
    return new SingleList<int>();
  else if (obj == "ts")
    return new TreiberStack<int>();
  else if (obj == "tsd")
    return new TSDeque<int,TSDequeBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  else if (obj == "tsq")
    return new TSQueue<int,TSQueueBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  else if (obj == "tss")
    return new TSStack<int,TSStackBuffer<int,HardwareTimestamp>,HardwareTimestamp>(g_num_threads+1, delay);
  else if (obj == "ukq")
    return new UnboundedSizeKFifo<int>(k);
  else if (obj == "wfq11")
    return new WaitfreeQueue<int>(g_num_threads+1);
  // else if (obj == "wfq12")
  //   return new WaitfreeQueue<int>(g_num_threads+1, max_retries, helping_delay);
  else
    assert(false && "Unexpected object name.");
}

string obj_name(string id) {
  if (objects.count(id) > 0)
    return objects[id].name;
  else
    return "???";
}

string obj_spec(string id) {
  if (objects.count(id) > 0)
    return objects[id].spec;
  else
    return "???";
}

void* scal_object_create(const char* id) {
  return static_cast<void*>(obj_create(string(id)));
}

const char* scal_object_name(const char* id) {
  return obj_name(string(id)).c_str();
}

const char* scal_object_spec(const char* id) {
  return obj_spec(string(id)).c_str();
}

void scal_object_put(void* obj, int v) {
  ensure_thread_initialized();
  static_cast<Pool<int>*>(obj)->put(v);
}

int scal_object_get(void* obj) {
  int result;
  ensure_thread_initialized();
  if (static_cast<Pool<int>*>(obj)->get(&result))
    return result;
  else
    return -1;
}
