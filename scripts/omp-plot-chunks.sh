#!/usr/bin/env bash

# @brief top directory
TOP_DIR="${0%/*}"

usage()
{
    cat <<EOF
Usage: ${0##*/} [options] csvtable

Plots scalability from CSV-table

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
        echo 'двухмерный массив'
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
plot_title=$(echo "Исследование влияния размера чанка при\nпараллелизации сортировки ${sort_impl}\n(матрицы - ${matrices_impl}, N = 64)")

# @brief output path
output="${csvtable/.csv/.png}"
output="${output/data/assets}"

gnuplot -p <<EOF
set ylabel 'Время, мс'
set xlabel 'Размер чанка'
set title "${plot_title}"
set style line 100 lt 1 lc rgb "grey" lw 0.5 # linestyle for the grid
set grid ls 100 # enable grid with specific linestyle
set datafile separator ','
set terminal png size 1280,720 enhanced font "Helvetica,14"
set output '${output}'
set key right top

set xtics 4
set xrange [1:128]

set style line 101 lw 3 lt rgb "#01afd2"

plot '${csvtable}' using 1:2 with lines ls 101 title "t = f(chunksize)"
EOF
echo "Saved PNG to ${output}"
