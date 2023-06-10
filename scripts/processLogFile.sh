#!/bin/bash

tallyResultsFile=$1
inputFile="../misc/poll-log.txt"

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
sort -k1,1 -k2,2 -fsu -o $inputFile $inputFile

partyNames=`cut -d " " -f3 $inputFile | sort | uniq`

# define an array with the parties
arrayname=( $partyNames)
 
# Create a the tally results file
: > $tallyResultsFile

# get item count using ${arrayname[@]}
for m in "${arrayname[@]}"
do
  # This will be the party
  line="${m}"
  line+=" "
  # This will be the number of occurencies of the party
  line+=`grep -w ${m} $inputFile | wc -l`
  echo "$line" >> "$tallyResultsFile"
done

echo "all went ok"