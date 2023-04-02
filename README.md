# bratley
C++ template metaprogramming implementation of Bratley's scheduling algorithm. The algorithm is computed at compile-time such that the schedule is known by run-time.

An example of the schedule tree generated using Bratley's scheduling algorithm can be seen in the figure below:

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
