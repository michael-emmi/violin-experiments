class Scheduler {
public:
  const static int DELAY = -2, DONE = -1;
public:
  virtual bool nextSchedule() = 0;
  virtual int nextStep() = 0;
  virtual void completed() = 0;
};

class RoundRobinScheduler : public Scheduler {
  const int num_threads;
  const int num_delays;
  int *delay_positions;
  deque<int> schedule;
  int step;
  int delay_count;

public:
  RoundRobinScheduler(vector<Thread> &ts, int delays)
    : num_threads(ts.size()), num_delays(delays) {

    delay_positions = new int[num_delays];
    delay_count = -1;
  }

  bool nextSchedule() {
    if (delay_count == 0)
      return false;

    else if (delay_count < 0) {
      for (int i=0; i<num_delays; i++)
        delay_positions[i] = i;

    } else {
      delay_positions[delay_count-1]++;
      for (int i=delay_count; i<num_delays; i++)
        delay_positions[i] = delay_positions[i-1] + 1;
    }

    delay_count = 0;
    step = 0;
    for (int i=0; i<num_threads; i++)
      schedule.push_back(i);
    return true;
  }

  int nextStep() {
    if (schedule.size() < 1)
      return DONE;
    
    if (schedule.size() > 1
        && delay_count < num_delays
        && delay_positions[delay_count] == step) {

      schedule.push_back(schedule.front());
      schedule.pop_front();
      delay_count++;
      step++;
      return DELAY;
    }

    step++;
    return schedule.front();
  }
  
  void completed() {
    schedule.pop_front();
  }
};

class AtomicScheduler : public Scheduler {
  const int num_threads;
  vector<int> schedule;
  int *c, *o;
  int turn;

public:
  AtomicScheduler(vector<Thread> &ts)
    : num_threads(ts.size()) {

    c = new int[num_threads];
    o = new int[num_threads];
    for (int i=0; i<num_threads; i++) {
      c[i] = 0;
      o[i] = 1;
    }
  }

  bool nextSchedule() {
    turn = 0;
    
    if (schedule.empty()) {
      for (int i=0; i<num_threads; i++)
        schedule.push_back(i);
      return num_threads > 0;
    }
    
    // "Plain changes" algorithm from Knuth, Combinatorial Algorithms (F2B)

    int j = num_threads-1;
    int s = 0;
    int q, x;

    ready_to_change:
      q = c[j] + o[j];
      if (q < 0) goto switch_direction;
      if (q == j+1) goto increase_s;
      
    change:
      x = schedule[j-c[j]+s];
      schedule[j-c[j]+s] = schedule[j-q+s];
      schedule[j-q+s] = x;
      c[j] = q;
      return true;
      
    increase_s:
      if (j == 0) return false;
      s++;
    
    switch_direction:
      o[j] = -o[j];
      j--;
      goto ready_to_change;
  }

  int nextStep() {
    if (turn >= num_threads)
      return DONE;

    return schedule[turn];
  }
  
  void completed() {
    turn++;
  }
};