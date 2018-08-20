Todos
=====

## Autotuning

- running-time normalization
  - first thing to try is put in user-interface to specify indicators of workload for normalization
  - then try to find that out automatically.
  - alternative: include the context in the model,
    and generate a decision tree in the code based on what the model learned dispatch to differently optimized functions based on inputs.
- integrate with Michael's LLVM branch
  - Generate fixed-dimensional loop-tiling knob
- Fix up Bayes tuner
  - cross-validation
  - reallocation
- Add more benchmarks
  - matrix multiply
  - some other stencil

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
