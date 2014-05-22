#!/usr/bin/python
# -*- coding: utf-8 -*-

import subprocess
import os
import re
import random
import sys
from math import log

grpFilePattern = re.compile(r'^G_(\d+)$')
methodPattern = re.compile(r'\s*--\s+(.*)$')
totalLog = []
number = '0'

def runSet(degree, k, grp):
    global totalLog
    filename = "S1_%04d_%02d" % (degree, k)
    grpGenerators = [l.strip() for l in open(grp).readlines()]
    grpGenerators = [line.replace(',',')(').replace(' ',',') for line in grpGenerators[2:]]
    grpGenerators = ['(%s)' % line for line in grpGenerators];
    totalLog.append('G:=Group(%s);;' % (','.join(grpGenerators)))
    #totalLog.append('SC:=StabChainMutable(G);;')
    totalLog.append('r1:=Runtimes();;')

    sets = [l.strip() for l in open(filename).readlines()]
    sets = sets[1:int(number)+1]
    for s in sets:
        #totalLog.append('SCtemp:=CopyStabChain(SC);;')
        #totalLog.append('ChangeStabChain(SCtemp,[%s]);;' % (s.replace(' ',',')))
        totalLog.append('Stabilizer(G,[%s],OnSets);;' % (s.replace(' ',',')))
    
    totalLog.append('r2:=Runtimes();;')
    totalLog.append('Print("%04d %02d ", r2.user_time - r1.user_time, "\\n");;' % (degree, k))

if len(sys.argv) < 2:
    print "no number given"
    print "usage:", sys.argv[0], "numberOfRuns"
    sys.exit(-1)
number = sys.argv[1]

for root, dirs, files in os.walk("."):
    files.sort()
    for f in files:
        m = grpFilePattern.match(f)
        if not m:
            continue

        file = open(f)
        _ = file.readline()
        degree = int(file.readline())
        file.close()
        
        k = int(log(degree, 2))
        for l in range(k-3, k+2):
            runSet(degree, l, f)

for l in totalLog:
    print l
