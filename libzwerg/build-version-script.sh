#!/bin/sh

if [ libzwerg.map -ot $1/libzwerg.map ]; then
    cp $1/libzwerg.map libzwerg.map && rm libzwerg.so
else
    true
fi
