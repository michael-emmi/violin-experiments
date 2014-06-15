/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/
#include <iostream>

class CountingMonitor : public Monitor {
protected:
  const int num_methods;
  const int interval_bound;
  int *counters;
  int time_offset, last_time;
  const bool debug = false;

public:
  CountingMonitor(int N, int M, bool collect)
    : Monitor("Operation-Counting", collect),
      interval_bound(N), num_methods(M), counters(new int[N*(N+1)*M]) { }

  virtual void onPreExecute() {
    Monitor::onPreExecute();
    memset(counters, 0, num_methods * interval_bound * (interval_bound+1) * sizeof(int));
    last_time = 0;
    time_offset = 0;
  }
  virtual void onPostExecute() {
    Monitor::onPostExecute();
  }
  virtual void onCall(Operation *op) { count(op); }
  virtual void onReturn(Operation *op) { count(op); }

protected:
  virtual int method(Operation *op) = 0;

  inline int idx(int m, int i, int j) {
    return
      m * interval_bound * (interval_bound+1)
      + i * (interval_bound+1)
      + j;
  }
  inline int offset(int m, int i, int j) {
    return
      m * interval_bound * (interval_bound+1) * sizeof(int)
      + i * (interval_bound+1) * sizeof(int)
      + j * sizeof(int);
  }

  void shift_counters() {
    for (int m=0; m<num_methods; m++) {
      for (int i=0; i<interval_bound; i++) {
        for (int j=0; j<interval_bound; j++) {
          if (i==0 && j==0)
            continue;
          int ti = i>0 ? i-1 : 0;
          int tj = j>0 ? j-1 : 0;
          counters[idx(m,ti,tj)] += counters[idx(m,i,j)];
          counters[idx(m,i,j)] = 0;
        }
      }
      for (int i=1; i<interval_bound; i++) {
        counters[idx(m,i-1,interval_bound)] += counters[idx(m,i,interval_bound)];
        counters[idx(m,i,interval_bound)] = 0;
      }
    }
  }

  void count(Operation *op) {
    int now = op->endTime();
    if (now == OMEGA) now = op->startTime();

    while (last_time < now) {
      last_time++;
      if (last_time >= interval_bound) {
        shift_counters();
        time_offset++;
      }
    }

    int start_time = op->startTime() - time_offset;
    if (start_time < 0) start_time = 0;

    if (op->endTime() == OMEGA) {
      counters[idx(method(op),start_time,interval_bound)]++;

    } else {
      int end_time = op->endTime();
      op->unend();
      counters[idx(method(op),start_time,interval_bound)]--;
      op->end(end_time);
      counters[idx(method(op),start_time,end_time-time_offset)]++;
    }
    if (debug) print_counters();
  }

  void print_counters() {
    cout << "--+";
    for (int j=0; j<interval_bound+1; j++)
      cout << "--";
    cout << endl;
    for (int m=0; m<num_methods; m++) {
      cout << "M" << m << "|";
      for (int j=0; j<interval_bound; j++)
        cout << " " << j;
      cout << " *" << endl;
      cout << "--+";
      for (int j=0; j<interval_bound+1; j++)
        cout << "--";
      cout << endl;
      for (int i=0; i<interval_bound; i++) {
        cout << i << " |";
        for (int j=0; j<interval_bound+1; j++) {
          if (i <= j)
            cout << " " << counters[idx(m,i,j)];
          else
            cout << " .";
        }
        cout << endl;
      }
      cout << "--+";
      for (int j=0; j<interval_bound+1; j++)
        cout << "--";
      cout << endl;
    }
  }

};
