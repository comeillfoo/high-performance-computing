# high-performance-computing

## Setup and Info

### OpenCL

Tools:

- C/C++ compiler: `gcc/g++`
- OpenCL headers: `sudo apt install -y opencl-headers`
- Installable Client Driver (ICD), shared library: `sudo apt install -y ocl-icd-opencl-dev`

Intel CPUs specific:

- [add intel repos](http://web.archive.org/web/20240901232236/https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html)
- `sudo apt install -y intel-oneapi-runtime-opencl intel-oneapi-runtime-libs`
- `sudo clinfo` should show that it support at least 1 platform, e.g. `Intel(R) OpenCL` and at least 1 device, e.g. `Intel(R) Core(TM) i5-1035G1 CPU @ 1.00GHz`

Sources:

1. [intel/compute-runtime](https://github.com/intel/compute-runtime)

Theory:

1. [Дизайн OpenCL](http://opencl.ru/design)

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
