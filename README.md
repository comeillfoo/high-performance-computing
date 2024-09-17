# high-performance-computing

## Setup and Info

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

Theory:

1. [Дизайн OpenCL](http://opencl.ru/design)
2. [OpenCL Programming model](https://github.com/KhronosGroup/OpenCL-Guide/blob/main/chapters/opencl_programming_model.md)
3. [OpenCL Book](https://fixstars.github.io/opencl-book/opencl-book/basic-opencl/basic-program-flow.html)

## System Description

- CPU: TODO
- RAM: TODO
- Caches: TODO

## Results

- $N_1$: shows the size of M1 when execution time exceeds 10 ms
- $N_1$: shows the size of M1 when execution time exceeds 5000 ms

### Plain stage results

- $N_1$: 438
- $N_2$: 10451

### OpenMP stage results

- $N_1$: 2
- $N_2$: 18607

### POSIX Threads stage results

For 4 threads:

- $N_1$: 2
- $N_2$: 19900

### OpenCL stage results
