set terminal pdf
set output "compare.pdf"
set title "Receive Rate and Packets Received"
set xlabel "Time (seconds) "
set ylabel "Receive Rate"
plot "AODV.csv" using 1:2 with points title "AODV", "OLSR.csv" using 1:2 with points title "OLSR", "DSR.csv" using 1:2 with points title "DSR", "DSDV.csv" using 1:2 with points title "DSDV"

set xlabel "Time (seconds) " 
set ylabel "Packets Received"
plot "AODV.sv" using 1:3 with points title "AODV", "OLSR.csv" using 1:3 with points title "OLSR","DSR.csv" using 1:3 with points title "DSR", "DSDV.csv" using 1:3 with points title "DSDV"