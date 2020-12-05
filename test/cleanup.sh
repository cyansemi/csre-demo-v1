#!/bin/bash
rm output/*

for f in $(find testdata -name *.supp); do
    rm $f
done
for f in $(find testdata -name *.unsupp); do
    rm $f
done
for f in $(find testdata -name *.msg); do
    rm $f
done

#special for simfiles which is a link.
for f in $(find testdata/simfiles/ -name *.supp); do
    rm $f
done
for f in $(find testdata/simfiles/ -name *.unsupp); do
    rm $f
done
for f in $(find testdata/simfiles/ -name *.msg); do
    rm $f
done
