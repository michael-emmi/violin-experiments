# Violin/Enumerator Experiments

### Current Status

* `bqk` / **Bounded-size K FIFO**  
  working

* `dq` / **Distributed Queue**  
  working; but careful: uses `pthread_getspecific`

* `dtsq` / **DTS Queue**  
  error -- segmentation fault; uses `pthread_getspecific`, thus incompatible

* `fcq` / **Flat-combining Queue**  
  blocking; uses `pthread_getspecific`, thus incompatible

* `ks` / **K Stack**  
  error -- segmentation fault

* `lbq` / **Lock-based Queue**  
  blocking :-)

* `msq` / **Michael-Scott Queue**  
  working

* `rdq` / **Random-dequeue Queue**  
  working

* `sl` / **Single List**  
  not thread-safe -- segmentation fault

* `ts` / **Treiber Stack**  
  working, and seemingly correct

* `tsd` / **TS Deque**  
  error -- malloc incorrect checksum for freed object

* `tss` / **TS Stack**  
  error -- malloc incorrect checksum for freed object

* `tsq` / **TS Queue**  
  error -- malloc incorrect checksum for freed object

* `ukq` / **Unbounded-size K FIFO**  
  working, violations even with atomic operations.

* `wfq11` / **Wait-free Queue (2011)**  
  error -- segmentation fault; uses `pthread_getspecific`, thus incompatible

* `wfq12` / **Wait-free Queue (2012)**  
  ??; uses `pthread_getspecific`, thus incompatible
