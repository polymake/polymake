#!/usr/bin/python

import subprocess
import os
import re
import random
import sys
from math import log

grpFilePattern = re.compile(r'^G_(\d+)$')
methodPattern = re.compile(r'\s*--\s+(.*)$')
totalLog = {}
cmd = 'false'
number = '0'

def runPair(grp1, grp2):
    global totalLog
    fgrp1 = "G_%04d" % grp1
    fgrp2 = "G_%04d" % grp2
    cout = subprocess.Popen([cmd, fgrp1, fgrp2, number], stdout=subprocess.PIPE, bufsize=1024).stdout
    label = None
    for i in range(0,100):
        line = cout.readline()
        if not line:
            break
        m = methodPattern.match(line)
        if not m:
            if label:
                if not totalLog.has_key(label):
                    totalLog[label] = []
                totalLog[label].append(("%dc%d " % (grp1,grp2)) + line)
        else:
            label = m.group(1)

if len(sys.argv) < 4:
    print "no command or number given"
    print "usage:", sys.argv[0], "cmd", "numberOfRuns", "pairsFile"
    sys.exit(-1)
cmd = sys.argv[1]
number = sys.argv[2]
pairs = sys.argv[3]

plines = [l.strip() for l in open(pairs).readlines()]
pairs = [map(int, l.split(' ')) for l in plines]
for a, b in pairs:
    runPair(a, b)

for label in totalLog.keys():
    print label
    for l in totalLog[label]:
        print l,
