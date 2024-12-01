#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

# @brief number of tests to make
TESTS_NR=10

# @brief high N limit
HIGH_N_LIMIT=100

# @brief threads number upper limit
HIGH_NUM_THREADS="$(($(nproc) * 2))"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] benchmark

Explores benchmark's scalability by changing threads number (OMP_NUM_THREADS).
Changes number of threads from 1 to ${HIGH_NUM_THREADS}

Options:
    -h, --help   Prints this help message
    -H, --high   Sets high limit for testing task size (N), default ${HIGH_N_LIMIT}
    -T, --tests  Number of tests to make, default ${TESTS_NR}
EOF
    exit 22 # EINVAL: Invalid argument
}

while true; do
    case $1 in
        -h|--help)
            usage
            ;;
        -T|--tests)
            [ "$2" -lt 1 ] && usage
            TESTS_NR="$2"
            shift 2
            ;;
        -H|--high)
            [ "$2" -lt 2 ] && usage
            HIGH_N_LIMIT="$2"
            shift 2
            ;;
        *)
            break
            ;;
    esac
done

[ $# -lt 1 ] && usage

# @brief path to program under measurements
benchmark="$1"

set -ueo pipefail

if [ ! -f "${benchmark}" ]; then
    echo "Unable to find benchmark ${benchmark}"
    exit 2 # ENOENT: No such file or directory
fi

export OMP_CANCELLATION=true
for threads_nr in $(seq 1 "${HIGH_NUM_THREADS}"); do
    export OMP_NUM_THREADS="${threads_nr}"
    "${TOP_DIR}/explore-task-scalability.sh" --tests "${TESTS_NR}" \
        --high "${HIGH_N_LIMIT}" "${benchmark}"
done
