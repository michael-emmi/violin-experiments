/*****************************************************************************/
/** COUNTING                                                                **/
/*****************************************************************************/

#include <map>

enum violin_op_t { ADD_OP, REMOVE_OP };

int ***counters;
int num_methods;
int (*__method)(violin_op_t,int,int);
int interval_bound;
int time_offset, last_time;

void __init_counters(int N, int M, int (*me)(violin_op_t,int,int)) {
  interval_bound = N+1;
  num_methods = M;
  __method = me;
  counters = new int**[M];
  for (int m=0; m<M; m++) {
    counters[m] = new int*[interval_bound];
    for (int i=0; i<interval_bound; i++) {
      counters[m][i] = new int[interval_bound+1];
    }
  }
}

void __shift_counters() {
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

void __reset_counters() {
  for (int m=0; m<num_methods; m++)
    for (int i=0; i<interval_bound; i++)
      for (int j=0; j<interval_bound+1; j++)
        counters[m][i][j] = 0;
  last_time = 0;
  time_offset = 0;
}

void __count(violin_op_t op, int v, int r, int start_time, int end_time = INFINITY) {
  int now = current_time();
  while (last_time < now) {
    last_time++;
    if (last_time > time_bound) {
      __shift_counters();
      time_offset++;
    }
  }
  
  start_time -= time_offset;
  if (start_time < 0) start_time = 0;
  
  int m1 = __method(op,v,UNKNOWN_VAL);
  int m2 = __method(op,v,r);
  if (end_time == INFINITY) {
    counters[m1][start_time][interval_bound]++;
  } else {
    counters[m1][start_time][interval_bound]--;
    counters[m2][start_time][end_time-time_offset]++;
  }
}

void __print_counters() {
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
