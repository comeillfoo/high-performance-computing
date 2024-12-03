#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] speedups

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

[ $# -lt 1 ] && usage

# @brief path to speedups
speedups_path="$1"
if [ ! -d "${speedups_path}" ]; then
    echo "No such directory ${speedups_path}"
    exit 2 # ENOENT: No such file or directory
fi
set -ueo pipefail

# @brief path to speedups
efficiencies_path="${speedups_path%/*}/efficiencies"
mkdir -p "${efficiencies_path}"
for threads in $(seq 1 8); do
    python3 "${TOP_DIR}/compute-props.py" -p "${threads}" efficiency \
        "${speedups_path}/${threads}.csv" \
        "${efficiencies_path}/${threads}.csv"
done
