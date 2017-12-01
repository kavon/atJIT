Easy::jit: just-in-time compilation for C++
-------------------------------------------

Finally! The wonders of just-in-time compilation are available in C++.
Easy::jit provides a simple interface over the LLVM's just-in-time compiler.

A call to ```easy::jit``` serves as specification for the generated code and
entry point for the just-in-time compiler.

..code-block:: cpp

  int baz(int a, int b) { ... }

  int foo(int a) {
    // compile a specialized version of baz
    auto baz_2 = easy::jit(baz, _0, 2); // mimics std::bind
    return baz_2(a); // run !
  }

The call to ```easy::jit``` generates a series of runtime library calls that are
later parsed by an especial LLVM pass. This pass embeds bitcode versions of
the C++ code in the binary.
A runtime library parses the bitcode and generates binary code based on the
library calls in the code.
