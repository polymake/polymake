#!/bin/sh

gcc -g -DGMP -DTIMES -DSIGNALS -O3 -c lrslib.c
gcc -g -DGMP -DTIMES -DSIGNALS -O3 -c lrsgmp.c

