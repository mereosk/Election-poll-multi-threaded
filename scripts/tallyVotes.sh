#!/bin/bash

tallyResultsFile=$1
inputFile="../misc/inputFile.txt"

# Check if the number of arguments is 1
if [ $# -ne 1 ]; then
    echo "Total number of arguments mush be 1."
    exit
fi

# Check if the inputFile exists in misc dir and if it has
# write permissions, if not exit
if [[ ! -f $inputFile ]]; then
    echo "inputFile does not exist"
    exit
elif [[ ! -w $inputFile ]]; then
    echo "inputFile does not have write perms"
    exit
fi

# If there are duplicate names the keep only the first vote
sort -k1,1 -fsu $inputFile | tee $inputFile

names=`cut -d " " -f '1 2' $inputFile`
echo "$names"

# i=1
# while read line; do
# # reading each line
# echo "Line No. $i : $line"
# i=$((i+1))
# done < $inputFile

echo "all went ok"