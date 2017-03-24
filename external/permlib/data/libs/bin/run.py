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
totalLog = {}
cmd = 'false'
number = '0'

def runSet(degree, k, grp):
    global totalLog
    filename = "S1_%04d_%02d" % (degree, k)
    print "running", cmd, 'at', filename
    cout = subprocess.Popen([cmd, grp, filename, number], stdout=subprocess.PIPE, bufsize=1024).stdout
    label = None
    for i in range(0,5000):
        line = cout.readline()
        if not line:
            break
        m = methodPattern.match(line)
        if not m:
            if label:
                if not totalLog.has_key(label):
                    totalLog[label] = []
                totalLog[label].append(line)
        else:
            label = m.group(1)

if len(sys.argv) < 3:
    print "no command or number given"
    print "usage:", sys.argv[0], "cmd", "numberOfRuns"
    sys.exit(-1)
cmd = sys.argv[1]
number = sys.argv[2]

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

print "------"

k = totalLog.keys()
k.sort()
for label in k:
    print label
    for l in totalLog[label]:
        print l,
