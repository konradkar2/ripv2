#/bin/bash

service openvswitch-switch start
ovs-vsctl set-manager ptcp:6640

mn --test pingpair 
if [[ $? -eq 0 ]]; then
    echo "Mininet works!"
fi

python3 -u ./mininet-scripts/HttpServerTest.py

sleep 500