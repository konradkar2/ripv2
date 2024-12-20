#!/usr/bin/python
from __future__ import annotations
import time
import pexpect
from os import system
from typing import Dict
import json

def wait_for_prompt(munet: pexpect.spawn, timeout=5):
    munet.expect("munet>", timeout)
    return munet


def run_munet(ns_path):
    munet = pexpect.spawn("munet -d {0}".format(ns_path))
    time.sleep(1)
    wait_for_prompt(munet, 20)

    output = munet.before.decode()
    print(output)
    return munet


class Host:
    def __init__(self, id: str, munet_ifc, address : str):
        self.id = id
        self.munet_ifc = munet_ifc
        self.address = address

    def to_string(self):
        return "id: {0}, address: {1}".format(self.id, self.address)

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet_ifc.sendline("{0} sh {1}".format(self.id, cmd))
        wait_for_prompt(self.munet_ifc, timeout)
        
        raw = self.munet_ifc.before.decode()
        raw_lines = raw.split('\n')
        return '\n'.join(raw_lines[1:])

    def execute_ip_route(self):
        routes_stdout = self.execute_shell("ip route").strip()
        routes = routes_stdout.split('\r\n')
        return [s.strip() for s in routes]

    def execute_rip_cli_r(self):
        rip_routes_stdout = self.execute_shell("rip-cli -r").strip()
        return rip_routes_stdout.split('\r\n')
    
    def has_connectivity(self, otherHost: Host, retries: int = 5, sleepTime: float = 1):
        for i in range(retries):
            ping_stdout = self.execute_shell("ping {0} -c 2 -w 1".format(otherHost.address))
            if "64 bytes from {0}".format(otherHost.address) in ping_stdout:
                return True
            time.sleep(sleepTime)

        print("No connectivity between host {0} and {1}".format(self.to_string(), otherHost.to_string()))
        return False

class MunetEnvironment:
    def __init__(self, env_name: str, hostIdToIpAddress):
        print("Starting munet (env_name: {})".format(env_name))
        self.env_name  = env_name;
        self.munet_ns_dir = "/tmp/ripv2_tests/{0}".format(env_name)
        system("rm -rf {0}".format(self.munet_ns_dir))
        self.munet_ifc = run_munet(self.munet_ns_dir)
        self.hosts = {}

        assert(len(hostIdToIpAddress) > 0)        
        for hostId in hostIdToIpAddress:
            ip_address = hostIdToIpAddress[hostId]
            self.hosts[hostId] = Host(hostId, self.munet_ifc, ip_address)

        time.sleep(5)

    
    def two_hosts_have_connectivity(self, hostIdA: str, hostIdB: str):
        hostA = self.hosts[hostIdA]
        hostB = self.hosts[hostIdB]
        return hostA.has_connectivity(hostB)
    
    def hosts_have_connectivity(self):
        hostPairs = []
        hostsIds = list(self.hosts.keys())

        for i in range(len(hostsIds)):
            for j in range(i +1, len(hostsIds)):
                hostAId = hostsIds[i]
                hostBId = hostsIds[j]
                pair = (self.hosts[hostAId], self.hosts[hostBId])
                hostPairs.append(pair)

        ret = True;

        connectivity_status = "Connectivity test: \n"
        for hostA, hostB in hostPairs:
            if hostA.has_connectivity(hostB, 30, 1):
                connectivity_status += "{0} -> {1} OK\n".format(hostA.id, hostB.id)
            else:
                ret = False
                connectivity_status += "{0} -> {1} NOK\n".format(hostA.id, hostB.id)
        
        print(connectivity_status)               
        return ret
    
    def get_log_filename(self, hostname):
        return "{0}/{1}/var.log.rip/rip.log".format(self.munet_ns_dir, hostname)

    def show_logs(self, hostname: str):
        filepath = self.get_log_filename(hostname)
        with open(filepath) as f:
            print("")
            print(f.read())
   

    def teardown_munet(self):
        print("teardown, cleaning munet")
        self.munet_ifc.sendline("quit")
        time.sleep(2)
        system("pkill munet")     



