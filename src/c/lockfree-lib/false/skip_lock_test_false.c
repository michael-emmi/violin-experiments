#include "skip_lock_wrapper_false.c"
#include "stdio.h"
#include "stdlib.h"
#include "pthread.h"


int tick;
int tickBound = 0;
pthread_mutex_t counterLock;

#define AddArg0Max 2
#define AddRetMax 2
int AddOpen[AddArg0Max];
int AddDone[AddArg0Max][AddRetMax];

#define RemArg0Max 2
#define RemRetMax 2
int RemOpen[RemArg0Max];
int RemDone[RemArg0Max][RemRetMax];


void checkInvariant () {
    assert (
        (RemDone[1][1] == 0 && AddDone[1][0] == 0 && AddDone[1][1] == 0 && ((RemDone[0][1] == 0 && AddDone[0][0] == 0 && AddDone[0][1] == 0 && RemOpen[1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] >= 0 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[0][0] >= 0) ||
 (RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= 0 && RemOpen[0] + RemDone[0][1] >= 1 && RemOpen[0] >= 0 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 0 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[0][0] >= 0 && AddDone[0][1] >= 0))) ||
 (RemDone[1][1] == 0 && AddDone[1][0] == 0 && AddDone[1][1] == 0 && ((AddDone[0][0] == 0 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] >= 0 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[0][1] >= 0) ||
 (RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] + RemDone[0][1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[0][0] >= 1 && AddDone[0][1] >= 0))) ||
 (RemDone[0][1] == 0 && AddDone[0][0] == 0 && AddDone[0][1] == 0 && RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= -1 && RemOpen[1] + RemDone[1][1] >= 0 && RemOpen[1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 1 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0) ||
 (RemDone[0][1] == 0 && AddDone[0][0] == 0 && AddDone[0][1] == 0 && RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= 0 && RemOpen[1] + RemDone[1][1] >= 1 && RemOpen[1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 0 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0) ||
 (AddDone[0][0] == 0 && RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= -1 && RemOpen[1] + RemDone[1][1] >= 0 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 1 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][1] >= 0) ||
 (AddDone[0][0] == 0 && RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= 0 && RemOpen[1] + RemDone[1][1] >= 1 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 0 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][1] >= 0) ||
 (RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= -1 && RemOpen[1] + RemDone[1][1] >= 0 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] + RemDone[0][1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 1 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][0] >= 1 && AddDone[0][1] >= 0) ||
 (RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= -1 && RemOpen[1] + RemDone[1][1] >= 0 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= 0 && RemOpen[0] + RemDone[0][1] >= 1 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 1 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 0 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][0] >= 0 && AddDone[0][1] >= 0) ||
 (RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= 0 && RemOpen[1] + RemDone[1][1] >= 1 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= -1 && RemOpen[0] + RemDone[0][1] >= 0 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 0 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 1 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][0] >= 1 && AddDone[0][1] >= 0) ||
 (RemOpen[1] + RemDone[1][1] - AddDone[1][1] >= 0 && RemOpen[1] + RemDone[1][1] >= 1 && RemOpen[1] >= 0 && RemOpen[0] + RemDone[0][1] - AddDone[0][1] >= 0 && RemOpen[0] + RemDone[0][1] >= 1 && RemOpen[0] >= 0 && AddOpen[1] + AddDone[1][1] - RemDone[1][1] >= 0 && AddOpen[1] + AddDone[1][1] >= 1 && AddOpen[1] >= 0 && AddOpen[0] + AddDone[0][1] - RemDone[0][1] >= 0 && AddOpen[0] + AddDone[0][1] >= 1 && AddOpen[0] >= 0 && RemDone[1][0] >= 0 && RemDone[1][1] >= 0 && RemDone[0][0] >= 0 && RemDone[0][1] >= 0 && AddDone[1][0] >= 0 && AddDone[1][1] >= 0 && AddDone[0][0] >= 0 && AddDone[0][1] >= 0)
    );
        
        
    
}

void* ticker(void *unused) {
    while(tick < tickBound) {
        tick++;
    }
    return NULL;
}

void atomicIncr(int *o) {
    pthread_mutex_lock(&counterLock);
    *o += 1;
    pthread_mutex_unlock(&counterLock);
}

void atomicDecrIncr(int *o, int *c) {
    pthread_mutex_lock(&counterLock);
    *o -= 1;
    *c += 1;
    checkInvariant();
    pthread_mutex_unlock(&counterLock);
}

void* instrAdd(void* temp) {
    int* tempArgs = (int*) temp;
    int localAdd1 = tempArgs[0];
    int tickStart = tick;
    atomicIncr(&AddOpen[localAdd1]);
    int localAdd_ret = Add(localAdd1);
    printf("Add returning %d;\n",localAdd_ret);
    atomicDecrIncr(&AddOpen[localAdd1],&AddDone[localAdd1][localAdd_ret]);
    return NULL;
}

void* instrRem(void* temp) {
    int* tempArgs = (int*) temp;
    int localRem1 = tempArgs[0];
    int tickStart = tick;
    atomicIncr(&RemOpen[localRem1]);
    int localRem_ret = Rem(localRem1);
    printf("Remove returning %d;\n",localRem_ret);
    atomicDecrIncr(&RemOpen[localRem1],&RemDone[localRem1][localRem_ret]);
    return NULL;
}


int arguments1[1];
int arguments2[1];
int arguments3[1];
int arguments4[1];


int main() {
    tick = 0;
    Init();
    pthread_t tid1, tid2, tid3, tid4, tid5;
    arguments1[0] = 1;
    pthread_create(&tid1,NULL,&instrAdd,arguments1);

    arguments2[0] = 1;
    pthread_create(&tid2,NULL,&instrAdd,arguments2);

    arguments3[0] = 1;
    pthread_create(&tid3,NULL,&instrRem,arguments3);

    arguments4[0] = 1;
    pthread_create(&tid4,NULL,&instrRem,arguments4);

    pthread_create(&tid5,NULL,&ticker,NULL);
    sleep(2);
}
