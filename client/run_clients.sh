#!/bin/bash

mkdir log_files

# Execute as: ./run_clients <server's ip address> <server's port>
./remoteClient -i $1 -p $2 -d dir1 2> ./log_files/log1 &
./remoteClient -i $1 -p $2 -d dir2 2> ./log_files/log2 &
./remoteClient -i $1 -p $2 -d dir3 2> ./log_files/log3 &
