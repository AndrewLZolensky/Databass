#!/bin/bash

make clean
make

./coordinator -v &
coordinator_pid=$!

./server -v -p 1 &
server1_pid=$!

./server -v -p 2 &
server2_pid=$!

./server -v -p 3 &
server3_pid=$!

./server -v -p 4 &
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