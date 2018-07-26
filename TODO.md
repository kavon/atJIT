Todos
=====

## Autotuning

- ~~more loop metadata knobs~~
- integrate with Michael's LLVM branch
  - generate polly knobs
- allow users to specify a tunable function parameter
  - create a range/set type usable by both tuner and user
- make the Feedback stderr cutoff before reoptimizing configurable.
- smarter navigation of the search space
  - ~~JIT only when we have feedback from last version~~
  - ~~integrate XGBoost~~
  - ~~create a basic SURF-like tuner~~
  - create a simulated annealing tuner
- running-time normalization
  - first thing to try is put in user-interface to specify indicators of workload for normalization
  - then try to find that out automatically.
  - alternative: include the context in the model,
    and generate a decision tree in the code based on what the model learned dispatch to differently optimized functions based on inputs.
- Improve Bayesian/SuRF tuner
  - add in exploitation (aka, nearby mutations) of the best configs
    so that we're not only doing random exploration.

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
