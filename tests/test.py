#!/usr/bin/python
import time
import pexpect
import pytest
from os import system


def wait_for_prompt(munet: pexpect.spawn, timeout=5):
    munet.expect("munet>", timeout)
    return munet


def run_munet(ns_path):
    munet = pexpect.spawn("munet -d {0}".format(ns_path))
    wait_for_prompt(munet, 15)

    output = munet.before.decode()
    print(output)
    return munet


class Host:
    def __init__(self, name: str, munet):
        self.name = name
        self.munet = munet

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet.sendline("{0} sh {1}".format(self.name, cmd))
        wait_for_prompt(self.munet, timeout)
        return self.munet.before.decode()


def has_connectivity(host: Host, address: str, retries: int = 4, sleep: float = 1):
    for i in range(retries):
        ping_stdout = host.execute_shell("ping {0} -c 5".format(address))
        if "64 bytes from {0}".format(address) in ping_stdout:
            return True
        time.sleep(1)

    print("No connectivity between host {0} and {1}".format(host.name, address))
    return False


def wait_for_frr(r4: Host):
    r6_address = "10.0.5.6"
    if not has_connectivity(r4, r6_address):
        raise Exception("FRR did not converge")


class munet_environment:
    def __init__(self):
        munet_ns_dir = "/tmp/test_ns"
        system("rm -rf {0}".format(munet_ns_dir))
        self.munet = run_munet(munet_ns_dir)
        system("tail -F {0}/r3/var.log.rip/rip.log &".format(munet_ns_dir))
    
        self.r3 = Host("r3", self.munet)

        r4 = Host("r4", self.munet)
        wait_for_frr(r4)
        time.sleep(1)

        if not has_connectivity(self.r3, "10.0.3.4"):
            raise Exception("somehow r3 can't reach r4")

        time.sleep(1)

    def __del__(self):
        print("teardown, cleaning munet")
        self.munet.sendline("quit")
        time.sleep(2)
        system("pkill munet")        


@pytest.fixture(scope="module")
def munet_env():
    return munet_environment()


def test_simple(munet_env):
    assert 1 == 1


def test_routing_table_updates(munet_env):
    munet_env.r3.execute_shell(
        "route -n add -net 240.0.0.0 netmask 255.255.255.255 dev eth0"
    )

    cli_route = munet_env.r3.execute_shell("rip-cli")
    assert "240.0.0.0" in cli_route
