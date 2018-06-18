atJIT: A just-in-time autotuning compiler for C++
==========================================

[![Build Status](https://travis-ci.org/kavon/atJIT.svg?branch=master)](https://travis-ci.org/kavon/atJIT)

About
-----

atJIT is an _extremely_ early-phase experiment in just-in-time autotuning that
<<<<<<< HEAD
is based on the Easy::jit project.
=======
is based on the Easy::jit project.
>>>>>>> 4a257f68d0ce2b32c21a236375aee94b49f6bdc5

[Easy::jit](https://github.com/jmmartinez/easy-just-in-time) is a
_compiler-assisted_ library that enables simple Just-In-Time
code generation for C++ codes.

Building
--------

First, install clang and LLVM.

```bash
apt install llvm-5.0-dev llvm-5.0-tools clang-5.0
```

Then, configure and compile the project.

```bash
mkdir build install
cd build
cmake -G Ninja -DCMAKE_INSTALL_PREFIX=../install \
    -DCMAKE_CXX_COMPILER=clang++-5.0 -DCMAKE_C_COMPILER=clang-5.0 ..
ninja install
```
<!--
cmake -DLLVM_DIR=/usr/lib/llvm-5.0/cmake <path_to_easy_jit_src>
cmake --build .
-->

To build the examples, install the [opencv](https://opencv.org/) library,
and add the flags ```-DEASY_JIT_EXAMPLE=1``` to the cmake command.

To enable benchmarking, install the [google benchmark](https://github.com/google/benchmark) framework,
and add the flags ```-DEASY_JIT_BENCHMARK=1 -DBENCHMARK_DIR=<path_to_google_benchmark_install>``` to the cmake command.

Everything is ready to go!

<!--
### Docker

If you want to give only a quick test to the project, everything is provided to use it with docker.
To do this, generate a Dockerfile from the current directory using the scripts in ```<path_to_easy_jit_src>/misc/docker```,
then generate your docker instance.

```bash
python3 <path_to_easy_jit_src>/misc/docker/GenDockerfile.py  <path_to_easy_jit_src>/.travis.yml > Dockerfile
docker build -t easy/test -f Dockerfile
docker run -ti easy/test /bin/bash
```
-->

Basic usage
-----------

### Compiling my project with atJIT

Look in your install directory for the `bin/easycc` executable, which is a
thin wrapper around `clang++` with the correct arguments to run the
clang plugin and dynamically link in the runtime system.
You can use `easycc` as if it were `clang++`.
Here's an example:

```bash
➤ install/bin/easycc tests/simple/int_a.cpp -o int_a
➤ ./int_a
inc(4) is 5
inc(5) is 6
inc(6) is 7
inc(7) is 8
```

<!--
Since the Easy::Jit library relies on assistance from the compiler, its
mandatory to load a compiler plugin in order to use it.
The flag ```-Xclang -load -Xclang <path_to_easy_jit_build>/bin/EasyJitPass.so```
loads the plugin.

The included headers require C++14 support, and remember to add the include directories!
Use ```--std=c++14 -I<path_to_easy_jit_src>/cpplib/include```.

Finaly, the binary must be linked against the Easy::Jit runtime library, using
```-L<path_to_easy_jit_build>/bin -lEasyJitRuntime```.

Putting all together we get the command bellow.

```bash
clang++-5.0 --std=c++14 <my_file.cpp> \
  -Xclang -load -Xclang /path/to/easy/jit/build/bin/bin/EasyJitPass.so \
  -I<path_to_easy_jit_src>/cpplib/include \
  -L<path_to_easy_jit_build>/bin -lEasyJitRuntime
```
-->

### Using atJIT inside my project

Consider the code below from a software that applies image filters on a video stream.
In the following sections we are going to adapt it to use the atJIT library.
The function to optimize is ```kernel```, which applies a mask on the entire image.

The mask, its dimensions and area do not change often, so specializing the function for
these parameters seems reasonable.
Moreover, the image dimensions and number of channels typically remain constant during
the entire execution; however, it is impossible to know their values as they depend on the stream.

```cpp
static void kernel(const char* mask, unsigned mask_size, unsigned mask_area,
                   const unsigned char* in, unsigned char* out,
                   unsigned rows, unsigned cols, unsigned channels) {
  unsigned mask_middle = (mask_size/2+1);
  unsigned middle = (cols+1)*mask_middle;

  for(unsigned i = 0; i != rows-mask_size; ++i) {
    for(unsigned j = 0; j != cols-mask_size; ++j) {
      for(unsigned ch = 0; ch != channels; ++ch) {

        long out_val = 0;
        for(unsigned ii = 0; ii != mask_size; ++ii) {
          for(unsigned jj = 0; jj != mask_size; ++jj) {
            out_val += mask[ii*mask_size+jj] * in[((i+ii)*cols+j+jj)*channels+ch];
          }
        }
        out[(i*cols+j+middle)*channels+ch] = out_val / mask_area;
      }
    }
  }
}

static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  kernel(mask, mask_size, mask_area, image.ptr(0,0), out->ptr(0,0), image.rows, image.cols, image.channels());
}
```

The main header for the library is ```easy/jit.h```, where the only core function
of the library is exported. This function is called -- guess how? -- ```easy::jit```.
We add the corresponding include directive them in the top of the file.

```cpp
#include <easy/jit.h>
```

With the call to ```easy::jit```, we specialize the function and obtain a new
one taking only two parameters (the input and the output frame).

```cpp
static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  using namespace std::placeholders;

  auto kernel_opt = easy::jit(kernel, mask, mask_size, mask_area, _1, _2, image.rows, image.cols, image.channels());
  kernel_opt(image.ptr(0,0), out->ptr(0,0));
}
```

#### Deducing which functions to expose at runtime

atJIT embeds the [LLVM bitcode](https://llvm.org/docs/LangRef.html)
representation of the functions to specialize at runtime in the binary code.
To perform this, the library requires access to the implementation of these
functions.
atJIT does an effort to deduce which functions are specialized at runtime,
still in many cases this is not possible.

In this case, it's possible to use the ```EASY_JIT_EXPOSE``` macro, as shown in
the following code,

```cpp
void EASY_JIT_EXPOSE kernel() { /* ... */ }
```

or using a regular expression during compilation.
The command bellow exports all functions whose name starts with "^kernel".

```bash
clang++ ... -mllvm -easy-export="^kernel.*"  ...
```

#### Caching

In parallel to the ```easy/jit.h``` header, there is ```easy/code_cache.h``` which
provides a code cache to avoid recompilation of functions that already have been
generated.

Bellow we show the code from previous section, but adapted to use a code cache.

```cpp
#include <easy/code_cache.h>
```

```cpp
static void apply_filter(const char *mask, unsigned mask_size, unsigned mask_area, cv::Mat &image, cv::Mat *&out) {
  using namespace std::placeholders;

  static easy::Cache<> cache;
  auto const &kernel_opt = cache.jit(kernel, mask, mask_size, mask_area, _1, _2, image.rows, image.cols, image.channels());
  kernel_opt(image.ptr(0,0), out->ptr(0,0));
}
```

License
-------

See file `LICENSE` at the top-level directory of this project.

Thanks
------

Special thanks to Quarkslab for their support on working in personal projects.

In addition, thanks to Serge Guelton and Juan Manuel Martinez Caamaño for
originally developing Easy::jit.
