#!/bin/bash

ip address add 10.0.3.4/24 dev eth0
ip address add 10.0.4.4/24 dev eth1
route -n add -net 224.5.0.0 netmask 255.255.255.255 dev eth0
route -n add -net 224.5.0.0 netmask 255.255.255.255 dev eth1
