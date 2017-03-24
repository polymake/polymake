#!/usr/bin/env python
import sys
n = int(sys.argv[1])

print "* %d-dim cube" % (n)
print "H-representation"
print "begin"
print " %d %d integer" % (2*n, n+1)
for i in range(0,n):
    print " ".join(['0'] + ['1' if (j % n == i) else '0' for j in range(0,n)])
for i in range(0,n):
    print " ".join(['1'] + ['-1' if (j % n == i) else '0' for j in range(0,n)])
print "end"
