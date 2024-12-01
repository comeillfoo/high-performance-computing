#!/usr/bin/env bash

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] benchmark N

Prints only milliseconds from running benchmark with N

Options:
    -h, --help  Prints this help message
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

# @brief path to program under measurements
benchmark="$1"

# @brief argument for benchmark N
N="$2"
set -ueo pipefail

if [ ! -f "${benchmark}" ]; then
    echo "Unable to find benchmark ${benchmark}"
    exit 2 # ENOENT: No such file or directory
fi

if [ "${N}" -lt 2 ]; then
    echo "Invalid value for N (${N}), should be not less than 2"
    exit 22 # EINVAL: Invalid argument
fi
"${benchmark}" "${N}" | tail -n1 | grep -Eo '[0-9]+$'
