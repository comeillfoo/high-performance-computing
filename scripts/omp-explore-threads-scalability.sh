#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

# @brief number of tests to make
TESTS_NR=10

# @brief high N limit
HIGH_N_LIMIT=100

# @brief threads number upper limit
HIGH_NUM_THREADS="$(($(nproc) * 2))"

# @brief threads number lower limit
LOW_NUM_THREADS=1

# @brief Makefile's target to build and clean benchmark
MK_BUILD_TARGET='omp-main'

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] [benchmark]

Explores benchmark's performance by changing threads number (OMP_NUM_THREADS).
Changes number of threads from ${LOW_NUM_THREADS} to ${HIGH_NUM_THREADS}. Also, sets OMP_SCHEDULE
='static,1' and OMP_CANCELLATION=true.

Options:
    -h, --help   Prints this help message
    -H, --high   Sets high limit for testing task size (N), default ${HIGH_N_LIMIT}
    -l, --low    Sets low limit for threads number, default ${LOW_NUM_THREADS}
    -T, --tests  Number of tests to make, default ${TESTS_NR}
EOF
    exit 22 # EINVAL: Invalid argument
}

while true; do
    case $1 in
        -h|--help)
            usage
            ;;
        -H|--high)
            [ "$2" -lt 2 ] && usage
            HIGH_N_LIMIT="$2"
            shift 2
            ;;
        -l|--low)
            [ "$2" -gt "${HIGH_NUM_THREADS}" ] && usage
            [ "$2" -lt 1 ] && usage
            LOW_NUM_THREADS="$2"
            shift 2
            ;;
        -T|--tests)
            [ "$2" -lt 1 ] && usage
            TESTS_NR="$2"
            shift 2
            ;;
        *)
            break
            ;;
    esac
done

MK_BUILD_TARGET="${1:-${MK_BUILD_TARGET}}"
set -ueo pipefail

export OMP_CANCELLATION=true
export OMP_SCHEDULE='static,1'
count=0
for threads_nr in $(seq "${LOW_NUM_THREADS}" "${HIGH_NUM_THREADS}"); do
    export OMP_NUM_THREADS="${threads_nr}"
    program_text="NR==1 {print \"THREADS,\"\$0; next} {print \"${threads_nr},\"\$0}"
    if [ "${count}" -gt 0 ]; then
        program_text="NR>1 {print \"${threads_nr},\"\$0}"
    fi
    "${TOP_DIR}/explore-matrix-sorts.sh" --tests "${TESTS_NR}" \
        --high "${HIGH_N_LIMIT}" "${MK_BUILD_TARGET}" \
        | awk -W interactive "${program_text}" -
    count=$((count + 1))
done
