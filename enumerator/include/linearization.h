/*****************************************************************************/
/** LINEARIZATION                                                           **/
/*****************************************************************************/
struct OP {
  int id;
  int start;
  int finish;
  bool operator< (const OP& o) const {
    return id < o.id;
  }
};
multiset<OP> operations;

void compute_linearizations() {
  queue< pair< vector<OP>, multiset<OP> > > work_list;
  vector< vector<OP> > linearizations;
  vector<OP> empty_seq;

  work_list.push(make_pair(empty_seq,operations));

  while (!work_list.empty()) {
    vector<OP> sequence = work_list.front().first;
    multiset<OP> remaining = work_list.front().second;
    work_list.pop();

    if (remaining.empty()) {
      linearizations.push_back(sequence);
      continue;
    }

    int min = 9999;
    for (multiset<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o)
      if (o->start < min)
        min = o->start;

    multiset<OP> minimals;
    for (multiset<OP>::iterator o = remaining.begin(); o != remaining.end(); ++o) {
      if (o->start == min) {
        minimals.insert(*o);
      }
    }

    for (multiset<OP>::iterator o = minimals.begin(); o != minimals.end(); ++o) {
      vector<OP> sequence2(sequence);
      multiset<OP> remaining2(remaining);
      sequence2.push_back(*o);
      remaining2.erase(*o);
      work_list.push(make_pair(sequence2,remaining2));
    }
  }

  hout << "(" << linearizations.size() << "Ls) ";

  // cout << endl;
  // for (vector< vector<OP> >::iterator lin = linearizations.begin(); lin != linearizations.end(); ++lin) {
  //   for (vector<OP>::iterator o = lin->begin(); o != lin->end(); ++o)
  //     cout << o->id << "(" << o->start << "," << o->finish << ")";
  //   cout << endl;
  // }
}
