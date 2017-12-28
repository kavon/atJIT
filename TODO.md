To Do
=====

Doing
-----

* Inlining of structures.
  - large structure return

Feature
-------

* Combine specialization of functions: see Issues.
* Serialization/Deserialization of object code.
* Better error handling : 
  - if a function is passed as parameter and cannot be extracted, print a warning,
  - if it is the target function, print an error.
* Inlining of functions.
  - instead of doing on the parameter inlining, do it on a after-optimization pass, to inline **any** concrete function pointer that may appear.
* Expansion of recursive calls.
* Hook function to modify the IR at runtime.
  - e.g ```easy::jit(foo, _2, _1, easy::after_optimization([](BasicBlock* B) { instrument_edges(B); return true; }))```;
* Drop body of functions that are not used

Documentation
-------------

* API doc
* Installing doc
* More complex usage example

Testing
-------

* Docker setup for testing
* Small set of benchmarks 
* Script to randomly transform calls in c++ code to calls to easy::jit for testing on large projects
* Test with:
  - member functions
  - function objects (jit operator() ?)
  - other architectures: ARM ?
  - other OS: osx ?
