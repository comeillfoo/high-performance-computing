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

# @brief sorting parallelization
sort_impl=$(case "${csvtable}" in
    (*RO*)
        echo 'только по строкам'
        ;;
    (*RC*)
        echo 'по строкам и столбцам'
        ;;
    (*)
        echo 'Unable to determine sorting parallelization'
        exit 1
        ;;
esac)

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
plot_title=$(echo "Исследование параллельного ускорения при\nпараллелизации сортировки ${sort_impl}\n(матрицы - ${matrices_impl})")

gnuplot -p <<EOF
set ylabel 'Ускорение'
set xlabel 'N'
set title "${plot_title}"
set style line 100 lt 1 lc rgb "grey" lw 0.5 # linestyle for the grid
set grid ls 100 # enable grid with specific linestyle
set datafile separator ','
set key autotitle columnhead # use the first line as title
set terminal qt size 1280,720 persist
set key right top

stats '${csvtable}'
N=STATS_columns
plot for [i=2:N] '${csvtable}' using 1:i with lines
EOF
