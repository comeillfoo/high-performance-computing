set ylabel yname
set xlabel xname
set title titel
set style line 100 lt 1 lc rgb "grey" lw 0.5 # linestyle for the grid
set grid ls 100 # enable grid with specific linestyle
set datafile separator ','
set key autotitle columnhead # use the first line as title

stats csvtable
N=STATS_columns
plot for [i=2:N] csvtable using 1:i with lines
