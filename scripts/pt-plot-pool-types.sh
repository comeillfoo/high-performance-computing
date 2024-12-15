#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] csvtable

Plots pool types from CSV-table

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
plot_title=$(echo "Исследование вариантов распределения задач при\nпараллелизации сортировки ${sort_impl}\n(матрицы - ${matrices_impl})")

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
set key right top

set style line 2 lw 3 lt rgb "#e40045"
set style line 3 lw 3 lt rgb "#ffc700"
set style line 4 lw 3 lt rgb "#2615b1"
set style line 5 lw 3 lt rgb "#61d600"
set style line 6 lw 3 lt rgb "#a6bc09"
set style line 7 lw 3 lt rgb "#f62aa0"
set style line 8 lw 3 lt rgb "#b8ee30"
set style line 9 lw 3 lt rgb "#248ea6"
set style line 10 lw 3 lt rgb "#f2d338"

stats '${csvtable}'
N=STATS_columns
plot for [i=2:N] '${csvtable}' using 1:i with lines ls i
EOF
echo "Saved PNG to ${output}"
