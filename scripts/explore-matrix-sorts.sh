#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

# @brief number of tests to make
TESTS_NR=16

# @brief high N limit
HIGH_N_LIMIT=100

# @brief low N limit
LOW_N_LIMIT=2

# @brief step for N
STEP=1

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] makefile-target

Explores benchmark's performance by changing sorting parallelization: only rows
or rows and columns. Benchmark is built by provided makefile-target and cleaned
with target 'clean-<makefile-target>'

Options:
    -h, --help   Prints this help message
    -H, --high   Sets high limit for testing task size (N), default ${HIGH_N_LIMIT}
    -L, --low    Sets low limit for testing task size (N), default ${LOW_N_LIMIT}
    -S, --step   Sets step for testing task size (N), default ${STEP}
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
        -L|--low)
            [ "$2" -lt 2 ] && usage
            LOW_N_LIMIT="$2"
            shift 2
            ;;
        -S|--step)
            [ "$2" -lt 1 ] && usage
            STEP="$2"
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

# @brief name of benchmark's build target
build_target="$1"

# @brief path to benchmark
benchmark="./${build_target}"

set -ueo pipefail
[ "${LOW_N_LIMIT}" -gt "${HIGH_N_LIMIT}" ] && usage
[ "$((HIGH_N_LIMIT - LOW_N_LIMIT))" -lt "${STEP}" ] && usage

# parallelize only rows sorting
make "clean-${build_target}" &>/dev/null
make clean-build &>/dev/null
make USERCFLAGS='-DPARALLEL_SORT_ONLY_ROWS' "${build_target}" &>/dev/null
if [ ! -f "${benchmark}" ]; then
    echo "Unable to find benchmark ${benchmark}"
    exit 2 # ENOENT: No such file or directory
fi
"${TOP_DIR}/explore-task-scalability.sh" --tests "${TESTS_NR}" \
    --low "${LOW_N_LIMIT}" \
    --step "${STEP}" \
    --high "${HIGH_N_LIMIT}" \
    "${benchmark}" \
    | awk -W interactive 'NR==1 {print "MATRIX SORT,"$0; next} {print "RO,"$0}' -

# parallelize rows and columns
make "clean-${build_target}" &>/dev/null
make clean-build &>/dev/null
make "${build_target}" &>/dev/null
if [ ! -f "${benchmark}" ]; then
    echo "Unable to find benchmark ${benchmark}"
    exit 2 # ENOENT: No such file or directory
fi
"${TOP_DIR}/explore-task-scalability.sh" --tests "${TESTS_NR}" \
    --low "${LOW_N_LIMIT}" \
    --step "${STEP}" \
    --high "${HIGH_N_LIMIT}" \
    "${benchmark}" \
    | awk -W interactive 'NR>1 {print "RC,"$0}' -
