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

Explores benchmark's performance that built and cleaned by specified
makefile-target (default ${MK_BUILD_TARGET}) by changing threads number
(OMP_NUM_THREADS). Changes number of threads from ${LOW_NUM_THREADS} to
${HIGH_NUM_THREADS}. Also, sets OMP_SCHEDULE='static,1' and OMP_CANCELLATION=true.

Options:
    -h, --help   Prints this help message
    -H, --high   Sets high limit for testing task size (N), default ${HIGH_N_LIMIT}
    -L, --low    Sets low limit for testing task size (N), default ${LOW_N_LIMIT}
    -S, --step   Sets step for testing task size (N), default ${STEP}
    -m, --min    Sets low limit for threads number, default ${LOW_NUM_THREADS}
    -M, --max    Sets high limit for threads number, default ${HIGH_NUM_THREADS}
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
        -m|--min)
            [ "$2" -gt "${HIGH_NUM_THREADS}" ] && usage
            [ "$2" -lt 1 ] && usage
            LOW_NUM_THREADS="$2"
            shift 2
            ;;
        -M|--max)
            [ "$2" -lt "${LOW_NUM_THREADS}" ] && usage
            [ "$2" -lt 1 ] && usage
            HIGH_NUM_THREADS="$2"
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
[ "${LOW_N_LIMIT}" -gt "${HIGH_N_LIMIT}" ] && usage
[ "$((HIGH_N_LIMIT - LOW_N_LIMIT))" -lt "${STEP}" ] && usage
[ "${LOW_NUM_THREADS}" -gt "${HIGH_NUM_THREADS}" ] && usage

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
        --low "${LOW_N_LIMIT}" \
        --step "${STEP}" \
        --high "${HIGH_N_LIMIT}" \
        "${MK_BUILD_TARGET}" \
        | awk -W interactive "${program_text}" -
    count=$((count + 1))
done
