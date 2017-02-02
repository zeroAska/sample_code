#!/bin/bash
clear
#read name
make clean && make $1 && ./$1
