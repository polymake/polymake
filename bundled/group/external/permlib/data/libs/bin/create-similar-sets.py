#!/usr/bin/python

import os
import re
import random
from math import log, pow, ceil

def createSet(degree, k):
    filename = "S2_%04d_%02d" % (degree, k)
    if not os.path.exists(filename):
        print "creating", filename
        file = open(filename, "w")
        omega = range(1, degree+1)
        myset = range(1, degree+1)
        random.shuffle(myset)
        file.write(str(k) + "\n")
        for i in range(0, 10000):
            insertPos = random.randint(0, pow(k,3))
            insertPos = int(pow(insertPos, 1/3.0))
            myset = myset[:insertPos] + random.sample(omega, k-insertPos)
            file.write(",".join([str(o) for o in myset]) + "\n")
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
        k0 = max(0, k - 3)
        for l in range(k0, k+2):
              createSet(degree, l)

