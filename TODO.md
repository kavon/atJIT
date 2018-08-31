Todos
=====

## Autotuning

* Running-time normalization
  - first thing to try is put in user-interface to specify indicators of workload for normalization
  - then try to find that out automatically.
  - alternative: include the context in the model,
    and generate a decision tree in the code based on what the model learned dispatch to differently optimized functions based on inputs.
* Add more benchmarks!
* Experimentation Budget for rate limiting.
  - Could use some budget calculation or an annealing system to reduce experimentation
  - Perhaps consider the difference between predicted and actual performance in Bayes.
* Inserting passes at different points in the PassManagerBuilder pipeline (i.e., less used or useful-to-run-more passes).
* More asynchrony in the compile job queue (notably, training in Bayes could be async).
* Use LLVM's PGO data collection insertion and make it available to optimization passes.
* Hyperparameter tuning of the Bayes tuner
* Function workload normalization
* Exporting / persisting results of tuning.
  - Could just dump the best human-readable configs to a file and look for it when
    constructing the tuner. Another harder option would be to generate an object file and dynamically link.

## JIT Compilation

### Known Issues

* Inlining of structures.
  - large structure return

### Testing

* Test with:
  - member functions
  - function objects (jit operator() ?)
  - other architectures: ARM ?
  - other OS: osx ?
