#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] csvtable

Plots speedups from CSV-table

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

# @brief path to CSV-table
csvtable="$1"
set -ueo pipefail

# @brief matrices implementation
matrices_impl=$(case "${csvtable}" in
    (*table*)
        echo 'двумерный массив'
        ;;
    (*vector*)
        echo 'одномерный массив'
        ;;
    (*)
        echo 'Unable to determine matrices implementation'
        exit 1
        ;;
esac)

# @brief plot title
plot_title=$(echo "Исследование эффективности распараллеливания сортировки\n(матрицы - ${matrices_impl})")

# @brief output path
output="${csvtable/.csv/.png}"
output="${output/data/assets}"

gnuplot -p <<EOF
set ylabel 'Время, мс'
set xlabel 'N'
set title "${plot_title}"
set style line 100 lt 1 lc rgb "grey" lw 0.5 # linestyle for the grid
set grid ls 100 # enable grid with specific linestyle
set datafile separator ','
set key autotitle columnhead # use the first line as title
set terminal png size 1280,720 enhanced font "Helvetica,14"
set output '${output}'
set key left top

set style line 2 lw 3 lt rgb "#348888"
set style line 3 lw 3 lt rgb "#f24405"

stats '${csvtable}'
N=STATS_columns
plot for [i=2:N] '${csvtable}' using 1:i with lines ls i
EOF
echo "Saved PNG to ${output}"
