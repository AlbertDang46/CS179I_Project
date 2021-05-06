#!/usr/bin/env sh

gcc write_table.c shmht.c -o write_table
gcc read_table.c shmht.c -o read_table
gcc destroy_table.c shmht.c -o destroy_table
