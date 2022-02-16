#!/bin/bash


net_name=`ibdev2netdev | awk '{print $5}'`
ip=`ifconfig 2>>/dev/null | grep 10.0.2. | awk '{print $2}'`
for net in $net_name; do
    str1=`ibdev2netdev | grep $net`
    str2=`cat /sys/class/net/$net/device/numa_node`
    echo ${ip} ${str1} numa-${str2}
done
# echo $net_name