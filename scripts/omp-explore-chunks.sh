#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

# @brief number of tests to make
TESTS_NR=10

# @brief N
N=100

# @brief high chunk size limit
HIGH_CHUNK_SIZE_LIMIT=$((N * 2))

# @brief default threads number
OMP_NUM_THREADS="$(nproc)"

# @brief Makefile's target to build and clean benchmark
MK_BUILD_TARGET='omp-main'

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] [makefile-build-target]

Explores benchmark's performance by changing schedule's chunk size (OMP_SCHEDULE)
from 1 to 2N (${HIGH_CHUNK_SIZE_LIMIT}). Also, sets OMP_CANCELLATION=true and OMP_NUM_THREADS=${OMP_NUM_THREADS}

Options:
    -h, --help       Prints this help message
    -N, --size       Task size (N), default ${N}
    -T, --tests      Number of tests to make, default ${TESTS_NR}
    -p, --threads    Number of threads to use, default ${OMP_NUM_THREADS}
EOF
    exit 22 # EINVAL: Invalid argument
}

explore_chunks()
{
    local benchmark="$1"
    local sortalgo="$2"
    if [ ! -f "${benchmark}" ]; then
        echo "Unable to find benchmark ${benchmark}"
        exit 2 # ENOENT: No such file or directory
    fi
    for mtype in 'table' 'vector'; do
        unset MATRIX_TYPE
        export MATRIX_TYPE="${mtype}"
        for test_nr in $(seq "${TESTS_NR}"); do
            echo -n "${sortalgo},${mtype},${N},${test_nr}"
            for chunk_size in $(seq 1 "${HIGH_CHUNK_SIZE_LIMIT}"); do
                unset OMP_SCHEDULE
                export OMP_SCHEDULE="static,${chunk_size}"
                echo -n ",$("${TOP_DIR}/mark-duration.sh" "${benchmark}" "${N}")"
            done
            echo
        done
    done
}

while true; do
    case $1 in
        -h|--help)
            usage
            ;;
        -N|--size)
            [ "$2" -lt 2 ] && usage
            N="$2"
            shift 2
            ;;
        -T|--tests)
            [ "$2" -lt 1 ] && usage
            TESTS_NR="$2"
            shift 2
            ;;
        -p|--threads)
            [ "$2" -lt 1 ] && usage
            OMP_NUM_THREADS="$2"
            shift 2
            ;;
        *)
            break
            ;;
    esac
done

HIGH_CHUNK_SIZE_LIMIT=$((N * 2))
MK_BUILD_TARGET="${1:-${MK_BUILD_TARGET}}"

# @brief path to built benchmark
benchmark="./${MK_BUILD_TARGET}"
set -ueo pipefail

export OMP_CANCELLATION=true
export OMP_NUM_THREADS

# print CSV-header
echo -n 'MATRIX SORT,MATRIX TYPE,N,#'
for chunk_size in $(seq 1 "${HIGH_CHUNK_SIZE_LIMIT}"); do
    echo -n ",${chunk_size}"
done
echo

# parallelize only rows sorting
make "clean-${MK_BUILD_TARGET}" &>/dev/null
make clean-build &>/dev/null
make USERCFLAGS='-DPARALLEL_SORT_ONLY_ROWS' "${MK_BUILD_TARGET}" &>/dev/null
explore_chunks "${benchmark}" 'RO'

# parallelize rows and columns
make "clean-${MK_BUILD_TARGET}" &>/dev/null
make clean-build &>/dev/null
make "${MK_BUILD_TARGET}" &>/dev/null
explore_chunks "${benchmark}" 'RC'
