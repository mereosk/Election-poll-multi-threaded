#!/bin/bash

politicalParties=$1
numLines=$2
inputFile="../misc/inputFile.txt"
greekNames="../misc/greekNames.txt"
greekSurnames="../misc/greekSurnames.txt"

# Check if the number of arguments is 2
if [ $# -ne 2 ]; then
    echo "Total number of arguments mush be 2."
    exit
fi

# Check if the politicalParties.txt which will be the 
# first argument exists
if [[ ! -f $politicalParties ]]; then
    echo "file does not exist (first argument)"
    exit
fi

# Check if the num of lines is a positive integer
if [[ ! $numLines =~ ^[\-0-9]+$ ]] || (( $numLines <= 0 )); then
    echo "num of lines is an integer (second argument)"
    exit
fi

# Create the file inputFile in the misc directory. If the file exists , 
# it will be truncated to zero. Otherwise, the file will be created
: > $inputFile

for i in `seq 1 $numLines`; do
    line=`shuf -n 1 $greekNames`
    line+=" "
    line+=`shuf -n 1 $greekSurnames`
    line+=" "
    line+=`shuf -n 1 $politicalParties`
    
    echo "$line" >> "$inputFile"
done

echo "We are cool"