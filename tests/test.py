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
    def __init__(self, name: str, munet, address : str):
        self.name = name
        self.munet = munet
        self.address = address

    def to_string(self):
        return "name: {0}, address: {1}".format(self.name, self.address)

    def execute_shell(self, cmd: str, timeout: int = 5) -> str:
        self.munet.sendline("{0} sh {1}".format(self.name, cmd))
        wait_for_prompt(self.munet, timeout)
        
        raw = self.munet.before.decode()
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


def wait_for_frr(r4: Host, r6: Host):
    if not has_connectivity(r4, r6):
        raise Exception("FRR did not converge")


class munet_environment:
    def __init__(self):
        munet_ns_dir = "/tmp/test_ns"
        system("rm -rf {0}".format(munet_ns_dir))
        self.munet = run_munet(munet_ns_dir)
        system("tail -F -n +1 {0}/r3/var.log.rip/rip.log &".format(munet_ns_dir))
    
        self.r1 = Host("r1", self.munet, "10.0.1.1")
        self.r3 = Host("r3", self.munet, "10.0.3.3")
        self.r4 = Host("r4", self.munet, "10.0.3.4")
        self.r5 = Host("r5", self.munet, "10.0.5.5")
        self.r6 = Host("r6", self.munet, "10.0.5.6")
        wait_for_frr(self.r4, self.r6)
        time.sleep(1)

        if not has_connectivity(self.r3, self.r4):
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
    rip_routes = rip_routes_stdout.split('\r\n')
    assert(len(rip_routes) == 3)

    assert("ifi 3, dev eth1, rfamily_id 2, rtag 512, network 10.0.5.0/24, nh 10.0.3.4, metric 3" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 512, network 10.0.4.0/24, nh 10.0.3.4, metric 2" in rip_routes)
    assert("ifi 2, dev eth0, rfamily_id 2, rtag 512, network 10.0.1.0/24, nh 10.0.2.2, metric 2" in rip_routes)

def remove_last_line_if_empty(lines):
    if lines and not lines[-1].strip():
        lines.pop()
    return lines

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_libnl(munet_env):
    nl_routes_stdout = munet_env.r3.execute_shell("rip-cli -n").strip()
    nl_routes_routes = nl_routes_stdout.split('\r\n')
    #liblns dumping functionality adds extra new line, so remove it
    nl_routes_routes = remove_last_line_if_empty(nl_routes_routes)
    nl_routes_routes = [s.strip() for s in nl_routes_routes]

    assert("inet 10.0.1.0/24 table main type unicast via 10.0.2.2 dev eth0" in nl_routes_routes)
    assert("inet 10.0.4.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert("inet 10.0.5.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert(len(nl_routes_routes) == 3)

@retry(AssertionError, tries=10, delay=5.0)
def test_r1_reaches_r6(munet_env):
    assert has_connectivity(munet_env.r1, munet_env.r6)

@retry(AssertionError, tries=10, delay=5.0)
def test_r1_reaches_r5(munet_env):
    assert has_connectivity(munet_env.r1, munet_env.r5)

