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

# @brief default threads number
PT_NUM_THREADS="$(nproc)"

# @brief Makefile's target to build and clean benchmark
MK_BUILD_TARGET='pt-main'

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] [makefile-build-target]

Explores benchmark's performance by changing pool type (PT_POOL_TYPE).
Also, sets PT_NUM_THREADS=${PT_NUM_THREADS}

Options:
    -h, --help       Prints this help message
    -H, --high       High limit for testing task size (N), default ${HIGH_N_LIMIT}
    -L, --low        Sets low limit for testing task size (N), default ${LOW_N_LIMIT}
    -S, --step       Sets step for testing task size (N), default ${STEP}
    -T, --tests      Number of tests to make, default ${TESTS_NR}
    -p, --threads    Number of threads to use, default ${PT_NUM_THREADS}
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
        -p|--threads)
            [ "$2" -lt 1 ] && usage
            PT_NUM_THREADS="$2"
            shift 2
            ;;
        *)
            break
            ;;
    esac
done

MK_BUILD_TARGET="${1:-${MK_BUILD_TARGET}}"
set -ueo pipefail
[ "${LOW_N_LIMIT}" -gt "${HIGH_N_LIMIT}" ] && usage
[ "$((HIGH_N_LIMIT - LOW_N_LIMIT))" -lt "${STEP}" ] && usage

export PT_NUM_THREADS
count=0
for pt_type in 'static' 'dynamic'; do
    unset PT_POOL_TYPE
    export PT_POOL_TYPE="${pt_type}"
    program_text="NR==1 {print \"POOL TYPE,\"\$0; next} {print \"${pt_type},\"\$0}"
    if [ "${count}" -gt 0 ]; then
        program_text="NR>1 {print \"${pt_type},\"\$0}"
    fi
    "${TOP_DIR}/explore-matrix-sorts.sh" --tests "${TESTS_NR}" \
        --low "${LOW_N_LIMIT}" \
        --step "${STEP}" \
        --high "${HIGH_N_LIMIT}" \
        "${MK_BUILD_TARGET}" \
        | awk -W interactive "${program_text}" -
    count=$((count + 1))
done
