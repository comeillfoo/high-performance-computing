# high-performance-computing

## Methods of measurements

1. Measure scalability while changing task size by changing `N`.
    + Measure scalability while changing number of threads in OpenMP by changing [OMP_NUM_THREADS](https://www.openmp.org/spec-html/5.0/openmpse50.html).

2. Parallel acceleration and efficiency can be measured by measuring executions times and using next equations:
    + $S(p) = \frac{V(p)}{V(1)}$ - parallel acceleration.
    + $E(p) = \frac{S(p)}{p}$ - parallel efficiency, where `p` is number of threads (cores, processes).

3. Efficiency of parallelizing matrices sort. By defining macro `PARALLEL_SORT_ONLY_ROWS` while compiling `sorts.c`. This can be done by assigning `USERCFLAGS` environment variable or setting it in command line while building with `make`.

### OpenMP

4. Change schedule by setting environment variable [OMP_SCHEDULE](https://www.openmp.org/spec-html/5.0/openmpse49.html).
5. Change chunk size by setting environment variable [OMP_SCHEDULE](https://www.openmp.org/spec-html/5.0/openmpse49.html).

### Pthreads

4. TODO: investigate how to implement fixed size thread pool

### OpenCL

4. Only CPU, cause GPU is impossible to pass to VBox.

## Parameters

```
Generate. A = S(8) * N(5) * M(10) = 400

Map M1. B = 7; X = 1 + (24 mod B) = 4
Map M2. B = 8; X = 1 + (24 mod B) = 1
Merge. B = 6; X = 1 + 24 mod 6 = 1
Sort. B = 4; X = 1 + 24 mod 4 = 1
```

## Setup and Info

### Just (single-threaded)

Run `just-main N`, where `N` is the number of rows for `M1` and columns for `M2`. The minimum value is 2.

### OpenMP

Run `omp-main` with `OMP_CANCELLATION=true`. Change number of threads with `OMP_NUM_THREADS` environment variable.

### OpenCL

Tools:

- C/C++ compiler: `gcc/g++`
- OpenCL headers: `sudo apt install -y opencl-headers`
- Installable Client Driver (ICD), shared library: `sudo apt install -y ocl-icd-opencl-dev`

Intel CPUs specific:

- [add intel repos](http://web.archive.org/web/20240901232236/https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html)
- install packages:

```
sudo apt install -y \
    intel-opencl-icd intel-oneapi-runtime-compilers-2024 \
    intel-oneapi-runtime-opencl-2024
# now project use own offline compiler (oclwc) but in the
# intel-oneapi-compiler-dpcpp-cpp-2024.2 package
# can be found Intel's compiler which is installed at
# `/opt/intel/oneapi/compiler/latest/bin/opencl-aot`
```

- `sudo clinfo` should show that it support at least 1 platform, e.g. `Intel(R) OpenCL` and at least 1 device, e.g. `Intel(R) Core(TM) i5-1035G1 CPU @ 1.00GHz`

Sources:

1. [intel/compute-runtime](https://github.com/intel/compute-runtime)
2. [Дизайн OpenCL](http://opencl.ru/design)
3. [OpenCL Programming model](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/opencl_programming_model.md)
4. [OpenCL Book](https://fixstars.github.io/opencl-book/opencl-book/basic-opencl/basic-program-flow.html)
