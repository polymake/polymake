#!/usr/bin/env python
import sys
n = int(sys.argv[1])

print "* %d-dim simplex" % (n)
print "H-representation"
print "begin"
print " %d %d integer" % (n+1, n+1)
for i in range(0,n):
    print " ".join(['0'] + ['1' if (j % n == i) else '0' for j in range(0,n)])
print " ".join(['1'] + ['-1' for j in range(0,n)])
print "end"
