#!/usr/bin/python
import time
import pytest
from retry import retry
from test_fixture import Host, has_connectivity, munet_environment, wait_for_frr

class test_basic_environment:
    def __init__(self):
        
        self.munet_env = munet_environment("test_basic")
        self.munet_ifc = self.munet_env.munet_ifc

        self.r1 = Host("r1", self.munet_ifc, "10.0.1.1")
        self.r2 = Host("r2", self.munet_ifc, "10.0.2.2")
        self.r3 = Host("r3", self.munet_ifc, "10.0.3.3")
        self.r4 = Host("r4", self.munet_ifc, "10.0.3.4")
        self.r5 = Host("r5", self.munet_ifc, "10.0.5.5")
        self.r6 = Host("r6", self.munet_ifc, "10.0.5.6")
        wait_for_frr(self.r4, self.r6)
        time.sleep(1)

        if not has_connectivity(self.r3, self.r4):
            raise Exception("somehow r3 can't reach r4")

        time.sleep(1)
        
    def __del__(self):
        self.munet_env.show_logs(self.r3.name)
 
@pytest.fixture(scope="module")
def test_env():
    return test_basic_environment()

def test_simple(test_env):
    assert 1 == 1

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_learned(test_env):
    rip_routes_stdout = test_env.r3.execute_shell("rip-cli -r").strip()
    rip_routes = rip_routes_stdout.split('\r\n')
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 3, dev eth1, rfamily_id 2, rtag 512, network 10.0.5.0/24, nh 10.0.3.4, metric 3" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 512, network 10.0.4.0/24, nh 10.0.3.4, metric 2" in rip_routes)
    assert("ifi 2, dev eth0, rfamily_id 2, rtag 512, network 10.0.1.0/24, nh 10.0.2.2, metric 2" in rip_routes)

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_static(test_env):
    rip_routes_stdout = test_env.r3.execute_shell("rip-cli -r").strip()
    rip_routes = rip_routes_stdout.split('\r\n')
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 2, dev eth0, rfamily_id 2, rtag 10, network 10.0.2.0/24, nh 0.0.0.0, metric 1" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 10, network 10.0.3.0/24, nh 0.0.0.0, metric 1" in rip_routes)

def remove_last_line_if_empty(lines):
    if lines and not lines[-1].strip():
        lines.pop()
    return lines

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_libnl(test_env):
    nl_routes_stdout = test_env.r3.execute_shell("rip-cli -n").strip()
    nl_routes_routes = nl_routes_stdout.split('\r\n')
    #liblns dumping functionality adds extra new line, so remove it
    nl_routes_routes = remove_last_line_if_empty(nl_routes_routes)
    nl_routes_routes = [s.strip() for s in nl_routes_routes]

    assert("inet 10.0.1.0/24 table main type unicast via 10.0.2.2 dev eth0" in nl_routes_routes)
    assert("inet 10.0.4.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert("inet 10.0.5.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert(len(nl_routes_routes) == 3)

@retry(AssertionError, tries=10, delay=5.0)
def test_connectivity(test_env):
    assert has_connectivity(test_env.r1, test_env.r6)
    assert has_connectivity(test_env.r1, test_env.r5)
    assert has_connectivity(test_env.r2, test_env.r5)
    assert has_connectivity(test_env.r2, test_env.r4)

