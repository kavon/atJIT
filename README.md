atJIT: A just-in-time autotuning compiler for C++
==========================================

[![Build Status](https://travis-ci.org/kavon/atJIT.svg?branch=master)](https://travis-ci.org/kavon/atJIT)

About
-----

atJIT is an early-phase experiment in online autotuning.

The code was originally based on the [Easy::jit](https://github.com/jmmartinez/easy-just-in-time) project.

Prerequisites
--------

Before you can build atJIT, ensure that your system has

- a C++ compiler with sufficient [C++17 support](https://en.cppreference.com/w/cpp/compiler_support#C.2B.2B17_features). This likely means GCC >= 7, or Clang >= 4, but slightly older versions may work.
- cmake >= 3.5
- to run the test suite (the `check` target), python 2.7 + [the lit package](https://pypi.org/project/lit/)

Then, do the following:

#### Step 1

Install a compatible version of clang and LLVM version 6.
To do this on Ubuntu 18.04, you can simply install using the following command.

```bash
sudo apt update
sudo apt install llvm-6.0-dev llvm-6.0-tools clang-6.0
```

For versions of Debian or Ubuntu that do not have version 6 available in the
default APT repositories, you can first add the appropriate APT repository
[from this list](http://apt.llvm.org/).

In order to use a clang + LLVM that was built from source, you will need to
configure the build of LLVM with special CMake options (e.g., we require RTTI).
We have collected these options in the `./cmake/LLVM.cmake` file, which can
be added to your usual
invocation of CMake when building LLVM with the `-C` flag like so:

```bash
cmake -C <path-to-atJit-root>/cmake/LLVM.cmake  .. other arguments ..
```


#### Step 2

Obtain and build XGBoost by running the following command:

```
./xgboost/get.sh
```

Building atJIT
--------

Once you have met the prerequisites, we can build atJIT.
Starting from the root of the project, the general build steps are:

```bash
mkdir build install
cd build
cmake -DCMAKE_INSTALL_PREFIX=../install ..
cmake --build . --target install
```

Once this completes, you can jump to the usage section. For special builds of
atJIT, see below.

##### Build Options

If you are using a custom LLVM that is not installed system-wide, you'll need to add `-DCMAKE_PREFIX_PATH=<path-to-where-LLVM-was-installed>` to the first CMake command above. For example, `-DCMAKE_PREFIX_PATH=~/bin/llvm6/install`.

To build the examples, install the [opencv](https://opencv.org/) library,
and add the flags ```-DEASY_JIT_EXAMPLE=1``` to the cmake command.

To enable benchmarking, install the [google benchmark](https://github.com/google/benchmark) framework,
and add the flags ```-DEASY_JIT_BENCHMARK=1 -DBENCHMARK_DIR=<path_to_google_benchmark_install>``` to the cmake command.

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

Look in your install directory for the `bin/atjitc` executable, which is a
thin wrapper around `clang++` with the correct arguments to run the
clang plugin and dynamically link in the atJIT runtime system.
You can use `atjitc` as if it were `clang++`, as it forwards its arguments to `clang++`.
Here's an example:

```bash
➤ install/bin/atjitc -O2 tests/simple/int_a.cpp -o int_a
➤ ./int_a
inc(4) is 5
inc(5) is 6
inc(6) is 7
inc(7) is 8
```

### Using atJIT inside my project

The C++ library to use atJIT is quite simple.
To get started, construct the driver for the autotuner:

```c++
#include <tuner/driver.h>

... {
  tuner::ATDriver AT;
  // ...
}
```

A single driver can handle the tuning of multiple functions.
The driver only exposes one, do-it-all, variadic method `reoptimize`.
To actually "drive" the autotuning process, you should ideally call
`reoptimize` before every function call.
Sometimes the tuner will JIT compile a new version, or it will return
an already-compiled version that needs further runtime measurements.

Given a tuner `AT`, `reoptimize` has the following generic usage:

```c++
  /* (1) return type */ F = AT.reoptimize(
                           /* (2) function to reoptimize */
                           /* (3) arguments to the function */
                           /* (4) options for the tuner */
                          );   
```

1. The return type of the function is some variant of `easy::FunctionWrapper<> const&`, which is a C++ function object that can be called like an ordinary function. The type depends on (2) and (3), and you can typically just write `auto const&` in its place.

2. The function to be optimized, which can be a template function if the type is specified.

3. The list of arguments must match the arity of the original function, though not all arguments need to have a real value.
Providing a runtime value will allow the JIT compiler to specialize based on the value, and [std::placeholders](https://en.cppreference.com/w/cpp/utility/functional/placeholders) (e.g., `std::placeholders::_1`) leave parameters as-is. For example:

```c++
// returns a function computing fsub(a, 1.0)
  easy::FunctionWrapper<float(float)> const& decrement =
    AT.reoptimize(fsub, _1, 1.0);

  // returns a function computing fsub(0.0, b)
  auto const& negate = AT.reoptimize(fsub, 0.0, _1);

  printf("dec(5) == %f\n", decrement(5));
  printf("neg(3) == %f\n", negate(3));
```

4. The main option for the tuner is what algorithm to use during the search. If no option is specified, the tuner currently will not perform any search.
To use the random search, we would specify `tuner::AT_Random` like so:

```c++
for (int i = 0; i < 100; ++i) {
    auto const& tunedSub7 =
        AT.reoptimize(fsub, _1, 7.0,
            easy::options::tuner_kind(tuner::AT_Random));

    printf("8 - 7 == %f\n", tunedSub7(8));
  }
```

See `doc/readme/simple_at.cpp` for the complete example we have walked through in this section.

<!--

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

-->

License
-------

See file `LICENSE` at the top-level directory of this project.

Acknowledgements
------

Special thanks to Serge Guelton and Juan Manuel Martinez Caamaño for
originally developing Easy::jit.
