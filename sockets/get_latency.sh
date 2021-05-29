#!/bin/bash

if $1 == server
then
    for i in $(seq 1000 1000 1000000)
    do
        ./server char $i
    done

    for i in $(seq 1000 1000 1000000)
    do
        ./server node $i
    done 
elif $1 == client
then
    for i in $(seq 1000 1000 1000000)
    do
        ./client char $i $2 >> latency_data.txt
        sleep 5
    done

    for i in $(seq 1000 1000 1000000)
    do
        ./client node $i $2 >> latency_data_tree.txt
        sleep 5
    done 
fi
