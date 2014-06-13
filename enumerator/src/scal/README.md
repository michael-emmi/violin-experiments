# Violin/Enumerator Experiments

### Working

* `bqk` / **Bounded-size K FIFO**  
  working

* `dq` / **Distributed Queue**  
  be careful: uses `pthread_getspecific`

* `msq` / **Michael-Scott Queue**  
  working

* `rdq` / **Random-dequeue Queue**  
  working

* `ts` / **Treiber Stack**  
  seemingly correct

* `ukq` / **Unbounded-size K FIFO**  
  violations even with atomic operations.

### Not Working

* `dtsq` / **DTS Queue**  
  error -- segmentation fault; uses `pthread_getspecific`, thus incompatible

* `fcq` / **Flat-combining Queue**  
  blocking; uses `pthread_getspecific`, thus incompatible

* `ks` / **K Stack**  
  error -- segmentation fault

* `lbq` / **Lock-based Queue**  
  blocking :-)

* `sl` / **Single List**  
  not thread-safe -- segmentation fault

* `tsd` / **TS Deque**  
  error -- malloc incorrect checksum for freed object

* `tss` / **TS Stack**  
  error -- malloc incorrect checksum for freed object

* `tsq` / **TS Queue**  
  error -- malloc incorrect checksum for freed object

* `wfq11` / **Wait-free Queue (2011)**  
  error -- segmentation fault; uses `pthread_getspecific`, thus incompatible

* `wfq12` / **Wait-free Queue (2012)**  
  ??; uses `pthread_getspecific`, thus incompatible
