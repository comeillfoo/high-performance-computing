#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] unparallelized parallelized

Computes speedups from unparallelized (single-threaded) and parallelized
execution times

Options:
    -h, --help       Prints this help message
EOF
    exit 22 # EINVAL: Invalid argument
}

while true; do
    case $1 in
        -h|--help)
            usage
            ;;
        *)
            break
            ;;
    esac
done

[ $# -lt 2 ] && usage

# @brief path unparallelized execution times
just_path="$1"
if [ ! -f "${just_path}" ]; then
    echo "Unable to find ${just_path}"
    exit 2 # ENOENT: No such file or directory
fi

# @brief path to parallelized execution times
threads_path="$2"
if [ ! -d "${threads_path}" ]; then
    echo "No such directory ${threads_path}"
    exit 2 # ENOENT: No such file or directory
fi
set -ueo pipefail

# @brief path to speedups
speedups_path="${threads_path%/*}/speedups"
mkdir -p "${speedups_path}"
for threads in $(seq 1 8); do
    parallelized_table="${threads_path}/${threads}.csv"
    python3 "${TOP_DIR}/compute-props.py" -p "${threads}" speedup \
        "${just_path}" \
        "${parallelized_table}" \
        "${speedups_path}/${threads}.csv"
done
