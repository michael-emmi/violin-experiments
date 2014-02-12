// #include "set.h"
// #include "gc.h"
#include "ptst.h"
#include "stdio.h"
#include "ptst.c"
// #include "gc.c"
#include "skip_lock_true.c"

set_t *s;

int *placeHolder;
int num_threads = 1;

int Add(int i) {
    setval_t v = set_update(s,(setkey_t) i,placeHolder,1);
    if (v == NULL) return 1;
    else return 0;
}

int Rem(int i) {
    setval_t v = set_remove(s,(setkey_t) i);
    if (v == NULL) return 0;
    else return 1;
}

void Init() {
    placeHolder = malloc(sizeof(int));
    _init_ptst_subsystem();
//     _init_gc_subsystem();
    _init_set_subsystem();
    s = set_alloc();
}

// int main() {
//     Init();
//     printf("Initialization finished\n");
//     printf("%d\n",Add(0));
//     printf("%d\n",Add(0));
//     printf("%d\n",Add(1));
//     printf("%d\n",Add(1));
//     printf("%d\n",Rem(0));
//     printf("%d\n",Rem(0));
//     printf("%d\n",Rem(1));
//     printf("%d\n",Rem(1));
//     printf("%d\n",Add(0));
//     printf("%d\n",Add(0));
//     printf("%d\n",Add(1));
//     printf("%d\n",Add(1));
// }