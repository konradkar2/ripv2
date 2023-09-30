#!/usr/bin/python
import time
import pexpect


def wait_for_prompt(munet: pexpect.spawn, timeout=5):
    munet.expect("munet>", timeout)
    return munet


def run_munet(ns_path):
    munet = pexpect.spawn("munet -d {0}".format(ns_path))
    wait_for_prompt(munet)

    output = munet.before.decode()
    print(output)
    return munet


def verify_substring_exist(string: str, substring: str, msg: str):
    if substring in string:
        print(msg)
    else:
        raise RuntimeError(
            "{0} - failed: '{1}' not found in '{2}'".format(msg, substring, string)
        )


class Host:
    def __init__(self, name: str, munet):
        self.name = name
        self.munet = munet

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet.sendline("{0} sh {1}".format(self.name, cmd))
        wait_for_prompt(self.munet, timeout)
        return self.munet.before.decode()


def verify_connectivity(host: Host, address: str, retries: int = 4, sleep: float = 1):

    for i in range(retries):
        try:
            ping_stdout = host.execute_shell("ping {0} -c 5".format(address))
            verify_substring_exist(
                ping_stdout,
                "64 bytes from {0}".format(address),
                "Connectivity between host {0} and {1}".format(host.name, address),
            )
            return
        except Exception as e:
            print("#{0} No connectivity between host {1} and {2}...".format(i,host.name, address))
            time.sleep(1)

    raise RuntimeError("No connectivity between host {0} and {1}".format(host.name, address))


def wait_for_frr(r2: Host):
    r4_address = "10.0.3.4"
    verify_connectivity(r2, r4_address)
            
if __name__ == "__main__":
    munet_ns_dir = "/tmp/test_ns"
    munet = run_munet(munet_ns_dir)

    r1 = Host("r1", munet)
    r1.execute_shell("touch rip.log")

    r2 = Host("r2", munet)
    wait_for_frr(r2)

    r1.execute_shell("ip address add 10.0.1.1/24 dev eth0")
    r1.execute_shell("route -n add -net 224.0.0.0 netmask 240.0.0.0 dev eth0")
    
    time.sleep(1)
    verify_connectivity(r1, "10.0.1.2")

    print(r1.execute_shell("sh -c 'nohup rip >> rip.log 2>&1 &'"))
    
    while(1):
        time.sleep(100)
