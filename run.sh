#!/bin/bash

make clean
make

./coordinator -v -c Res/ipconfig.txt -g 2 &
coordinator_pid=$!

./server -v -p 9001 -i 0.0.0.0 &
server1_pid=$!

./server -v -p 9002 -i 0.0.0.0 & &
server2_pid=$!

./server -v -p 9003 -i 0.0.0.0 & &
server3_pid=$!

./server -v -p 9004 -i 0.0.0.0 & &
server4_pid=$!

wait_for_exit() {
  kill $coordinator_pid $server1_pid $server2_pid $server3_pid $server4_pid 2>/dev/null
  wait $coordinator_pid $server1_pid $server2_pid $server3_pid $server4_pid
  make clean
  exit 0
}

trap wait_for_exit SIGINT SIGTERM

wait $coordinator_pid $server1_pid $server2_pid  $server3_pid $server4_pid

echo "Waiting for processes"