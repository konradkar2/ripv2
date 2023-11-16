#!/usr/bin/python
import time
import pexpect
import pytest
from os import system
from retry import retry



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
    def __init__(self, name: str, munet):
        self.name = name
        self.munet = munet

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet.sendline("{0} sh {1}".format(self.name, cmd))
        wait_for_prompt(self.munet, timeout)
        
        raw = self.munet.before.decode()
        raw_lines = raw.split('\n')
        return '\n'.join(raw_lines[1:])


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

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_intermediate(munet_env):
    rip_routes_stdout = munet_env.r3.execute_shell("rip-cli -r").strip()
    rip_routes = rip_routes_stdout.split('\n')
    assert(len(rip_routes) == 3)

def remove_last_line_if_empty(lines):
    if lines and not lines[-1].strip():
        lines.pop()
    return lines

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_libnl(munet_env):
    nl_routes_stdout = munet_env.r3.execute_shell("rip-cli -n").strip()
    nl_routes_routes = nl_routes_stdout.split('\n')
    #liblns dumping functionality adds extra new line, so remove it
    nl_routes_routes = remove_last_line_if_empty(nl_routes_routes)
    assert(len(nl_routes_routes) == 3)

