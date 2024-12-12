#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

# @brief number of tests to make
TESTS_NR=16

# @brief high N limit
HIGH_N_LIMIT=100

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] benchmark

Explores benchmark's performance (scalability) by changing task size (N)

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
        -H|--high)
            [ "$2" -lt 2 ] && usage
            HIGH_N_LIMIT="$2"
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

[ $# -lt 1 ] && usage

# @brief path to program under measurements
benchmark="$1"

set -ueo pipefail

if [ ! -f "${benchmark}" ]; then
    echo "Unable to find benchmark ${benchmark}"
    exit 2 # ENOENT: No such file or directory
fi

# print CSV-table header
echo -n 'MATRIX TYPE,#'
for N in $(seq 2 "${HIGH_N_LIMIT}"); do
    echo -n ",${N}"
done
echo

# print CSV-table body
for mtype in 'table' 'vector'; do
    unset MATRIX_TYPE
    export MATRIX_TYPE="${mtype}"
    for test_nr in $(seq "${TESTS_NR}"); do
        echo -n "${mtype},${test_nr}"
        for N in $(seq 2 "${HIGH_N_LIMIT}"); do
            echo -n ",$("${TOP_DIR}/mark-duration.sh" "${benchmark}" "${N}")"
        done
        echo
    done
done
