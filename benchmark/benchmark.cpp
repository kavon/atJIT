
#include <benchmark/benchmark.h>
#include <easy/code_cache.h>
#include <tuner/driver.h>
#include <tuner/param.h>

#include <numeric>
#include <algorithm>
#include <cassert>
#include <iostream>


// benchmark components go here

#include "matmul.h"

#include "qsort.h"


BENCHMARK_MAIN();
