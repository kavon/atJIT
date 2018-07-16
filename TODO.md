Todos
=====

## Autotuning

- more loop metadata knobs
- integrate with Michael's LLVM branch.
- do the tunable parameter thing
- refinement of the search space
- running-time normalization
  - first thing to try is put in user-interface to specify indicators of workload for normalization
  - then try to find that out automatically.


## JIT Compilation

### Known Issues

* Inlining of structures.
  - large structure return

### Testing

* Small set of benchmarks
* Test with:
  - member functions
  - function objects (jit operator() ?)
  - other architectures: ARM ?
  - other OS: osx ?
