#!/bin/bash

addr=$(head -1 ../memcached.conf)
port=$(awk 'NR==2{print}' ../memcached.conf)

# kill old me
ssh ${addr} "cat /tmp/memcached.pid | xargs kill"

# launch memcached
ssh ${addr} "memcached -u root -l ${addr} -p  ${port} -c 10000 -d -P /tmp/memcached.pid"
sleep 1

# init 
echo -e "set serverNum 0 0 1\r\n0\r\nquit\r" | nc ${addr} ${port}
echo -e "set clientNum 0 0 1\r\n0\r\nquit\r" | nc ${addr} ${port}
