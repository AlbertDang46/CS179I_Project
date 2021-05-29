#!/bin/bash

for i in $(seq 1000 1000 1000000)
do
    while pgrep -x server; do sleep 5; done;
    ./server char $i &
    ./client char $i $1 >> latency_data.txt
done

for i in $(seq 1000 1000 1000000)
do
    while pgrep -x server; do sleep 5; done;
    ./server node $i &
    ./client node $i $1 >> latency_data_tree.txt
done 
