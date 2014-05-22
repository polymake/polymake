#!/usr/bin/env python
import sys
n = int(sys.argv[1])
l = int(sys.argv[2])

print "* (%d,%d)-cyclic polytope" % (n,l)
print "V-representation"
print "begin"
print " %d %d integer" % (l, n+1)
for i in range(0,l):
    print " ".join(['1'] + [str((i+1)**j) for j in range(1, n+1)])
print "end"
