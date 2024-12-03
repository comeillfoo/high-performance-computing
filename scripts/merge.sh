#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] source [source ...]

Merges resulting columns into single table

Options:
    -h, --help       Prints this help message
EOF
    exit 22 # EINVAL: Invalid argument
}

csv_filename()
{
    local path="$1"
    local base="${path##*/}"
    echo "${base%.csv}"
}

csv_thead_renamed()
{
    local path="$1"
    awk "NR==1 {print \"N,$(csv_filename "${path}")\"; next} {print \$0}" "${path}"
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

# @brief path to source tables
sources=($@)

# @brief length of sources array
n="${#sources[@]}"

if [ ! -f "${sources[0]}" ]; then
    echo "Unable to find ${sources[0]}"
    exit 2 # ENOENT: No such file or directory
fi

prev=$(csv_thead_renamed "${sources[0]}")
if [ "${n}" -gt 1 ]; then
    for i in $(seq 1 $((n - 1))); do
        curr=$(join -t',' -1 1 -2 1 <(echo "${prev}") <(csv_thead_renamed "${sources[${i}]}"))
        prev="${curr}"
    done
fi
echo "${prev}"
