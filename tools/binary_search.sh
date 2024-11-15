#!/usr/bin/env bash

# @brief target path
TARGET='../main'

# @brief lookup limit
LIMIT="${1:-11}"

# @brief lower limit
LOW="${2:-2}"

# @brief upper limit
HIGH="${3:-20000}"

# @brief loops
LOOPS=${4:-50}

function parse_time()
{
    local stdin=''
    while read stdin; do
        echo "${stdin}"
    done | tail -n1 - | grep -Eo 'passed: [0-9]+' | cut -d' ' -f2 -
}

if [ ! -f "${TARGET}" ]; then
    echo "Cannot found ${TARGET}"
    exit 2 # ENOENT
fi

while [ "${LOW}" -le "${HIGH}" ]; do
    mid=$((LOW + (HIGH - LOW) / 2))
    acc=0
    for _ in $(seq "${LOOPS}"); do
        milliseconds=$(${TARGET} ${mid} |& parse_time)
        echo "${mid} ${LOW} ${HIGH}: ${milliseconds}"
        acc=$((acc + milliseconds))
    done

    acc=$((acc / LOOPS))
    if [ "${acc}" -eq "${LIMIT}" ]; then
        break
    fi

    if [ "${acc}" -lt "${LIMIT}" ]; then
        LOW=$((mid + 1))
    else
        HIGH=$((mid - 1))
    fi
done

echo "It takes N=${mid} to overcome ${LIMIT}. Check: ${acc}"
