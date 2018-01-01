#!/bin/bash

TF=$(mktemp)
REDIR=/dev/stdout
for i in $(seq 10); do
    /usr/bin/time -a -o $TF -f %e -- dwgrep/dwgrep -c $1 -e "$2" > $REDIR
    sleep .5
    REDIR=/dev/null
done
python3 /dev/stdin "$TF"<<"EOF"
import sys
import statistics
l = list (float (l.strip ()) for l in open (sys.argv[1]))
b = min (l)
m = statistics.mean (l)
d = statistics.stdev (l)
print ("%s runs, best:%ss, avg:%ss, stddev:%s" % (len (l), b, m, d))
EOF

rm $TF
