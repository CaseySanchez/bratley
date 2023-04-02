# bratley
C++ template metaprogramming implementation of Bratley's scheduling algorithm. The algorithm is computed at compile-time such that the schedule is known by run-time.

An example of the schedule tree generated using Bratley's scheduling algorithm can be seen in the figure below, where `J_i` are the tasks to be scheduled, `a_i` are the arrival times, `C_i` are the computation times, and `d_i` are the deadlines:

![](bratley_schedule_tree.png)

Build instructions:
```
cd bratley
mkdir build
cd build
cmake ..
make
```

Live demo on the Godbolt Compiler Explorer:

https://godbolt.org/z/P4G9fjnvh
