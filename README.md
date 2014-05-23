"Violin" Experiments
====

New experiments in `enumerator`
----
This works by enumerating all schedules with a given number of barriers up to a
given delay bound. So far we have some working experiments from [scal]. To work
with them, run the following:

    cd enumerator/src/scal
    make
    ./scal --help

You will generally need to add `Yield` calls to the code in `datastructures`,
and specify flags like `-adds`, `-removes`, and `-delays` to `./scal`.

[scal]: http://scal.cs.uni-salzburg.at

Boogie examples from previous attempts, in `with-Boogie/src/bpl`
----
Including hand-written examples of Baskets Queue, Elimination Stack,
etc.

C examples from previous attempts, in `with-Boogie/src/c`
----
* `lfds2` (??)
* `lfds611` (??)
* `lockfree-lib`
* `LockFreeDS`
* `msqueue` - Michael & Scott's Queues
* `nbds.0.4.3` (??)
* `tbb` - from Intel's Thread Building Blocks
