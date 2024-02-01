#!/usr/bin/python
import time
import pexpect
from os import system


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
    def __init__(self, name: str, munet_ifc, address : str):
        self.name = name
        self.munet_ifc = munet_ifc
        self.address = address

    def to_string(self):
        return "name: {0}, address: {1}".format(self.name, self.address)

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet_ifc.sendline("{0} sh {1}".format(self.name, cmd))
        wait_for_prompt(self.munet_ifc, timeout)
        
        raw = self.munet_ifc.before.decode()
        raw_lines = raw.split('\n')
        return '\n'.join(raw_lines[1:])


def has_connectivity(hostA: Host, hostB: Host, retries: int = 4, sleep: float = 1):
    for i in range(retries):
        ping_stdout = hostA.execute_shell("ping {0} -c 2 -w 1".format(hostB.address))
        if "64 bytes from {0}".format(hostB.address) in ping_stdout:
            return True
        time.sleep(1)

    print("No connectivity between host {0} and {1}".format(hostA.to_string(), hostB.to_string()))
    return False


def wait_for_frr(router_a: Host, router_b: Host):
    if not has_connectivity(router_a, router_b):
        raise Exception("FRR did not converge")


class munet_environment:
    def __init__(self, env_name: str):
        self.munet_ns_dir = "/tmp/ripv2_tests/{0}".format(env_name)
        system("rm -rf {0}".format(self.munet_ns_dir))
        self.munet_ifc = run_munet(self.munet_ns_dir)
        time.sleep(2)
    
    def show_logs(self, hostname: str):
        filepath = "{0}/{1}/var.log.rip/rip.log".format(self.munet_ns_dir, hostname)
        with open(filepath) as f:
            print("")
            print(f.read())
        

    def __del__(self):
        print("teardown, cleaning munet")
        self.munet_ifc.sendline("quit")
        time.sleep(2)
        system("pkill munet")        


