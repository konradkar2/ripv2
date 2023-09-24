#!/usr/bin/python
from time import sleep
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


def verify_connectivity(host: Host, address: str):
    ping_stdout = host.execute_shell("ping {0} -c 5".format(address))
    verify_substring_exist(
        ping_stdout,
        "64 bytes from {0}".format(address),
        "Connectivity between host {0} and {1}".format(host.name, address),
    )


def wait_for_frr(r2: Host):
    print(r2.execute_shell("ip a"))

    r4_address = "10.0.3.4"
    for i in range(5):
        try:
            verify_connectivity(r2, r4_address)
            return
        except Exception as e:
            print("No connectivity via FRR #{0}...".format(i))
            sleep(2)

    raise RuntimeError("No connectivity between r2 and {0} (FRR)".format(r4_address))


if __name__ == "__main__":
    munet_ns_dir = "/tmp/test_ns"
    munet = run_munet(munet_ns_dir)

    r1 = Host("r1", munet)
    r2 = Host("r2", munet)
    wait_for_frr(r2)

    r1.execute_shell("ip address add 10.0.1.1/24 dev eth0")
    print(r1.execute_shell("ip a"))
