#!/usr/bin/python
import time
import pytest
from retry import retry
from base import Host, munet_environment
import conftest


class test_basic_environment(munet_environment):
    def __init__(self, name: str):

        hostMapping = {
        "r1": "10.0.1.1",
        "r2": "10.0.2.2",
        "r3": "10.0.3.3",
        "r4": "10.0.3.4",
        "r5": "10.0.5.5",
        "r6": "10.0.5.6"
        }
        super().__init__(name, hostMapping)
       
        if not self.two_hosts_have_connectivity("r3", "r4"):
            raise Exception("r3 can't reach r4")
        
    def __del__(self):
        self.show_logs("r3")
 
@pytest.fixture(scope="module")
def test_env(pytestconfig):
    return test_basic_environment(pytestconfig.getoption("name"))

def test_simple(test_env):
    assert 1 == 1

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_learned(test_env):
    rip_routes_stdout = test_env.hosts["r3"].execute_shell("rip-cli -r").strip()
    rip_routes = rip_routes_stdout.split('\r\n')
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.5.0/24, nh 10.0.3.4, metric 3" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.4.0/24, nh 10.0.3.4, metric 2" in rip_routes)
    assert("ifi 2, dev eth0, rfamily_id 2, rtag 0, network 10.0.1.0/24, nh 10.0.2.2, metric 2" in rip_routes)

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_static(test_env):
    rip_routes_stdout = test_env.hosts["r3"].execute_shell("rip-cli -r").strip()
    rip_routes = rip_routes_stdout.split('\r\n')
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 2, dev eth0, rfamily_id 2, rtag 0, network 10.0.2.0/24, nh 0.0.0.0, metric 1" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.3.0/24, nh 0.0.0.0, metric 1" in rip_routes)

def remove_last_line_if_empty(lines):
    if lines and not lines[-1].strip():
        lines.pop()
    return lines

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_libnl(test_env):
    nl_routes_stdout = test_env.hosts["r3"].execute_shell("rip-cli -n").strip()
    nl_routes_routes = nl_routes_stdout.split('\r\n')
    #liblns dumping functionality adds extra new line, so remove it
    nl_routes_routes = remove_last_line_if_empty(nl_routes_routes)
    nl_routes_routes = [s.strip() for s in nl_routes_routes]

    assert("inet 10.0.1.0/24 table main type unicast via 10.0.2.2 dev eth0" in nl_routes_routes)
    assert("inet 10.0.4.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert("inet 10.0.5.0/24 table main type unicast via 10.0.3.4 dev eth1" in nl_routes_routes)
    assert(len(nl_routes_routes) == 3)

def test_connectivity(test_env):
    assert test_env.hosts_have_connectivity()

def test_remove_dead_routes(test_env):
    test_env.hosts["r6"].execute_shell("pkill frr")

