#!/usr/bin/python
# -*- coding: utf-8 -*-

import os
import re
import random
from math import log

def createSet(degree, k):
    filename = "S1_%04d_%02d" % (degree, k)
    if not os.path.exists(filename):
        print "creating", filename
        file = open(filename, "w")
        omega = range(1, degree+1)
        file.write(str(k) + "\n")
        for i in range(0, 10000):
            file.write(",".join([str(o) for o in random.sample(omega, k)]) + "\n")
        file.close()

grpFilePattern = re.compile(r'G_(\d+)')

for root, dirs, files in os.walk("."):
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
             createSet(degree, l)
        
