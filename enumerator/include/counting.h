/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/
#include <iostream>

class CountingMonitor : public Monitor {
protected:
  const int num_methods;
  const int interval_bound;
  int ***counters;
  int time_offset, last_time;

public:
  CountingMonitor(int N, int M) : Monitor("Operation-Counting"), interval_bound(N), num_methods(M) {
    counters = new int**[M];
    for (int m=0; m<M; m++) {
      counters[m] = new int*[interval_bound];
      for (int i=0; i<interval_bound; i++) {
        counters[m][i] = new int[interval_bound+1];
      }
    }
  }
  virtual void onPreExecute() {
    Monitor::onPreExecute();
    for (int m=0; m<num_methods; m++)
      for (int i=0; i<interval_bound; i++)
        for (int j=0; j<interval_bound+1; j++)
          counters[m][i][j] = 0;
    last_time = 0;
    time_offset = 0;
  }
  virtual void onPostExecute() {
    Monitor::onPostExecute();
  }
  virtual void onCall(int op_id, violin_op_t op, int val, int start_time) {
    count(op,val,UNKNOWN_VAL,start_time);
  }
  virtual void onReturn(
      int op_id, violin_op_t op, int val, int ret_val,
      int start_time, int end_time) {
    count(op,val,ret_val,start_time,end_time);
  }

protected:
  virtual int method(violin_op_t op, int i, int j) = 0;

  void shift_counters() {
    for (int m=0; m<num_methods; m++) {
      for (int i=0; i<interval_bound; i++) {
        for (int j=0; j<interval_bound; j++) {
          if (i==0 && j==0)
            continue;
          int ti = i>0 ? i-1 : 0;
          int tj = j>0 ? j-1 : 0;
          counters[m][ti][tj] += counters[m][i][j];
          counters[m][i][j] = 0;
        }
      }
      for (int i=1; i<interval_bound; i++) {
        counters[m][i-1][interval_bound] += counters[m][i][interval_bound];
        counters[m][i][interval_bound] = 0;
      }
    }
  }

  void count(violin_op_t op, int v, int r, int start_time, int end_time = INFINITY) {
    int now = current_time();
    while (last_time < now) {
      last_time++;
      if (last_time > time_bound) {
        shift_counters();
        time_offset++;
      }
    }
  
    start_time -= time_offset;
    if (start_time < 0) start_time = 0;
  
    int m1 = method(op,v,UNKNOWN_VAL);
    int m2 = method(op,v,r);
    if (end_time == INFINITY) {
      counters[m1][start_time][interval_bound]++;
    } else {
      counters[m1][start_time][interval_bound]--;
      counters[m2][start_time][end_time-time_offset]++;
    }
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
            cout << " " << counters[m][i][j];
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
