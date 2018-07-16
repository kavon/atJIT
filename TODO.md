Todos
=====

## Autotuning

- ~~more loop metadata knobs~~
- integrate with Michael's LLVM branch
- allow users to specify a tunable function parameter
  - create a range/set type usable by both tuner and user.
- smarter navigation of the search space
  - ~~JIT only when we have feedback from last version~~
  - integrate XGBoost
  - create a SURF tuner
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
