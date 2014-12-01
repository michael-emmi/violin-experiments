#include "datastructures/pool.h"

#ifndef SCAL_INTERFACE
#define SCAL_INTERFACE 

using namespace std;

extern "C" {
  void* scal_object_create(const char* id);
  const char* scal_object_name(const char* id);
  const char* scal_object_spec(const char* id);

  void scal_initialize(unsigned num_threads);
  void scal_object_put(void*,int);
  int scal_object_get(void*);
}

struct obj_desc {
  string id;
  string name;
  string spec;
};

extern map<string,obj_desc> objects;

Pool<int>* obj_create(string obj);
string obj_name(string id);
string obj_spec(string id);

void scal_initialize(unsigned num_threads);
void scal_object_put(void* obj, int v);
int scal_object_get(void* obj);

void dilly_dally();

#ifndef Yield
  #define Yield() dilly_dally();
#endif

#endif
