/*****************************************************************************/
/** MEMORY ALLOCATION (TO CATCH ABA)                                        **/
/*****************************************************************************/

#include <deque>
#include <set>

enum violin_alloc_policy_t { DEFAULT_ALLOC, LRF_ALLOC, MRF_ALLOC };
violin_alloc_policy_t alloc_policy;

deque<void*> free_pool;
set<void*> alloc_set;

void* violin_malloc(int size) {
  void *x;
  if (free_pool.empty()) {
    x = malloc(size);
    alloc_set.insert(x);
  } else {
    x = free_pool.front();
    free_pool.pop_front();
  }
  return x;
}

void violin_free(void *x) {
  if (alloc_set.find(x) == alloc_set.end())
    return;

  switch (alloc_policy) {
  case LRF_ALLOC:
    free_pool.push_back(x);
    break;
  case MRF_ALLOC:
    free_pool.push_front(x);
    break;
  default:
    free(x);
    alloc_set.erase(x);
  }
}

void violin_clear_alloc_pool() {
  for (set<void*>::iterator p = alloc_set.begin(); p != alloc_set.end(); ) {
    // Tricky tricky! can't free(*p) before ++p
    void *ptr = *p;
    ++p;
    free(ptr);
  }
  alloc_set.clear();
  free_pool.clear();
}
