#!/bin/sh

src=$1; shift;
target=$1; shift;
dualabi="$@"

rm -f $target/*.c $target/*.h
cp $src/*.c $src/*.h $target/

for file in $dualabi
do
  in_file="$src/$file"
  out_file="$target/$(echo $file | sed -e 's/\./_f./')"
  sed -e 's/dd_/ddf_/g' \
    -e 's/cddf_/cdd_/g' \
    -e 's/mytype/myfloat/g' \
    -e 's/#include "cdd.h"/#include "cdd_f.h"/' \
    -e 's/#include "cddtypes.h"/#include "cddtypes_f.h"/' \
    -e 's/#include "cddmp.h"/#include "cddmp_f.h"/' \
    -e 's/__CDD_H/__CDD_HF/' \
    -e 's/__CDD_HFF/__CDD_HF/' \
    -e 's/__CDDMP_H/_CDDMP_HF/' \
    -e 's/__CDDTYPES_H/_CDDTYPES_HF/' \
    -e 's/GMPRATIONAL/ddf_GMPRATIONAL/g' \
    -e 's/ARITHMETIC/ddf_ARITHMETIC/g' \
    -e 's/CDOUBLE/ddf_CDOUBLE/g' \
    $in_file | awk 'BEGIN{print "/* generated automatically from $in_file */"}1' \
    > $out_file
done
