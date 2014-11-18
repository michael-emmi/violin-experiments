#include <gflags/gflags.h>
#include <sstream>
#include <map>

// NOTE order matters here, since Yield will be defined differently
#include "violin.h"
#include "scal.h"

DEFINE_int32(adds, 1, "how many add operations?");
DEFINE_int32(removes, 1, "how many remove operations?");
DEFINE_int32(barriers, 0, "how many barriers?");
DEFINE_int32(delays, 0, "how many delays?");
DEFINE_string(mode, "counting", "which mode? {nothing,counting,counting-no-verify,linearization,versus}");
DEFINE_int32(alloc, 0, "allocation policy? 0=default, 1=LRF, 2=MRF");
DEFINE_string(show, "all", "show which histories? {all,wins,violations,none}");

Pool<int> *obj, *spec_obj;
string lib_object, spec_object;

void obj_reset() {
  if (obj) delete obj;
  obj = obj_create(lib_object);
}

void spec_reset() {
  if (spec_obj) delete spec_obj;
  spec_obj = obj_create(spec_object);
}

void obj_add(int v) {
  scal_object_put(obj,v);
}

void spec_add(int v) {
  scal_object_put(spec_obj,v);
}

int obj_remove() {
  return scal_object_get(obj);
}

int spec_remove() {
  return scal_object_get(spec_obj);
}

violin_order_t obj_order(string id) {
  string spec = obj_spec(id);
  if (spec.find("stack") != string::npos)
    return LIFO_ORDER;
  else if (spec.find("queue") != string::npos)
    return FIFO_ORDER;
  else
    return NO_ORDER;
}

int main(int argc, char **argv) {

	scal_initialize(1);

  stringstream usage;
  usage << "usage" << endl;
  usage << "  " << argv[0] << " [flags] OBJ, where OBJ comes from:" << endl;
  int len = sizeof(objects) / sizeof(obj_desc);
  for (map<string,obj_desc>::iterator I = objects.begin(), E = objects.end(); I != E; ++I)
    usage << "    " << I->second.id << " -> " << I->second.name << endl;
  usage << "  then see the flags below.";

  google::SetUsageMessage(usage.str());
  google::ParseCommandLineFlags(&argc, &argv, true);

  if (argc-1 != 1) {
    cerr << "Must specify one data structure; see --help for usage." << endl;
    exit(-1);
  }

  if (objects.count(lib_object = argv[1]) == 0) {
    cerr << "Invalid data structure name \"" << argv[1] << "\"; see --help for usage." << endl;
    exit(-1);
  }

  spec_object = (obj_order(lib_object) == FIFO_ORDER) ? "msq" : lib_object;

  violin_mode_t mode;
  if (FLAGS_mode.find("none") != string::npos)
    mode = NOTHING_MODE;
  else if (FLAGS_mode.find("-skip-atomic") != string::npos)
    mode = LIN_SKIP_ATOMIC_MODE;
  else if (FLAGS_mode.find("lin") != string::npos)
    mode = LINEARIZATIONS_MODE;
  else if (FLAGS_mode.find("versus") != string::npos)
    mode = VERSUS_MODE;
  else if (FLAGS_mode.find("-no-verify") != string::npos)
    mode = COUNTING_NO_VERIFY_MODE;
  else
    mode = COUNTING_MODE;

  violin_alloc_policy_t alloc;
  switch (FLAGS_alloc) {
  case 0: alloc = DEFAULT_ALLOC; break;
  case 1: alloc = LRF_ALLOC; break;
  default: alloc = MRF_ALLOC; break;
  }

  violin_show_t show;
  if (FLAGS_show.find("none") != string::npos)
    show = SHOW_NONE;
  else if (FLAGS_show.find("viol") != string::npos)
    show = SHOW_VIOLATIONS;
  else if (FLAGS_show.find("win") != string::npos)
    show = SHOW_WINS;
  else
    show = SHOW_ALL;

  cout << "Selected SCAL data structure: " << obj_name(lib_object) << endl;
  violin(
    {.initialize = obj_reset, .add = obj_add, .remove = obj_remove},
    {.initialize = spec_reset, .add = spec_add, .remove = spec_remove},
    FLAGS_adds,
    FLAGS_removes,
    mode,
    alloc,
    obj_order(lib_object),
    FLAGS_barriers,
    FLAGS_delays,
    show
  );
  return 0;
}