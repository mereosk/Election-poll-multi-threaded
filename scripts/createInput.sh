#!/bin/bash

# This function generates a random string 3-12 characters long
generate_rand_name() {
    # Random number between 3-12
    len=$(( RANDOM % 10 + 3 ))

    # Define the chars that can be used in the random string
    lowerCharacters="abcdefghijklmnopqrstuvwxyz"
    upperCharacters="ABCDEFGHIJKLMNOPQRSTUVWXYZ"

    randName=""
    # First character will be uppercase
    randChar="${upperCharacters:RANDOM%${#upperCharacters}:1}"
    randName="${randName}${randChar}"
    # All the other characters will be lowercase
    counter=0
    while [ $counter -lt $len ]; do
        randChar="${lowerCharacters:RANDOM%${#lowerCharacters}:1}"
        randName="${randName}${randChar}"
        counter=$((counter + 1))
    done

    # Output the random string
    echo "$randName"
}

politicalParties=$1
numLines=$2
inputFile="../misc/inputFile.txt"

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
    line="$(generate_rand_name)"
    line+=" "
    line+="$(generate_rand_name)"
    line+=" "
    line+=`shuf -n 1 $politicalParties`
    
    echo "$line" >> "$inputFile"
done

echo "We are cool"
