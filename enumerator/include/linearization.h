/*****************************************************************************/
/** LINEARIZATION                                                           **/
/*****************************************************************************/

#include <vector>
#include <queue>
#include <unordered_set>
#include <set>

struct OP {
  Operation *op;
  int start() { return op->start_time; }
  int finish() { return op->end_time; }
  string to_string() {
    stringstream s;
    if (op->proc == Add)
      s << op->id << ":Add(" << op->parameter << ")? "
        << op->id << ":Add!";
    else if (op->proc == Remove) {
      s << op->id << ":Rem? "
        << op->id << ":Rem!";
      if (op->result == EMPTY_VAL) s << "E";
      else s << op->result;
    } else
      s << "????";
    return s.str();
  }
  bool operator< (const OP& o) const {
    return op->id < o.op->id;
  }
  bool operator== (const OP& o) const {
    return op->id == o.op->id;
  }
};

unordered_set<string> linear_histories;

void add_to_histories() {
  linear_histories.insert(hout.str());
}

void compute_sequential_histories() {
  cout << "Computing sequential histories... ";
  register_post(add_to_histories);
  time_t start_time, end_time;
  time(&start_time);
  search_atomic_threads();
  time(&end_time);
  unregister_post(add_to_histories);
  cout << linear_histories.size() << " histories computed in " 
       << difftime(end_time,start_time) << "s." << endl;
}

string linearization_to_string(vector<OP> l) {
  stringstream s;
  for (vector<OP>::iterator op = l.begin(); op != l.end(); ++op)
    s << op->to_string() << " ";
  return s.str();
}

void compute_linearizations() {
  list<OP> operations;
  queue< pair< vector<OP>, list<OP> > > work_list;
  vector< vector<OP> > linearizations;
  vector<OP> empty_seq;

  for (vector<Operation*>::iterator op = violin_operations.begin();
      op != violin_operations.end(); ++op)
    operations.push_back({.op = *op});

  work_list.push(make_pair(empty_seq,operations));

  while (!work_list.empty()) {
    vector<OP> sequence = work_list.front().first;
    list<OP> remaining = work_list.front().second;
    work_list.pop();

    if (remaining.empty()) {
      if (linear_histories.find(linearization_to_string(sequence))
          != linear_histories.end()) {
        // cout << ":-) " << linearization_to_string(sequence) << endl;
        return;
      } else {
        // cout << ":-X " << linearization_to_string(sequence) << endl;
        linearizations.push_back(sequence);
        continue;
      }
    }

    int min = INFINITY;
    for (list<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o)
      if (o->finish() < min)
        min = o->finish();

    list<OP> minimals;
    for (list<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o) {
      if (o->start() <= min) {
        minimals.push_back(*o);
      }
    }

    for (list<OP>::iterator o = minimals.begin(); o != minimals.end(); ++o) {
      vector<OP> sequence2(sequence);
      list<OP> remaining2(remaining);
      sequence2.push_back(*o);
      remaining2.remove(*o);
      work_list.push(make_pair(sequence2,remaining2));
    }
  }
  
  hout << "(LV) ";
  num_violations++;
  violation_happened = true;

  // hout << "(" << linearizations.size() << "Ls) ";

  // cout << endl;
  // for (vector< vector<OP> >::iterator lin = linearizations.begin(); lin != linearizations.end(); ++lin) {
  //   for (vector<OP>::iterator o = lin->begin(); o != lin->end(); ++o)
  //     cout << o->id << "(" << o->start << "," << o->finish << ")";
  //   cout << endl;
  // }
}
