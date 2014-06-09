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
  RoundRobinScheduler(vector<Thread> ts, int delays)
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
