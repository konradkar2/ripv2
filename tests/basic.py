#!/usr/bin/python
import time
import pytest
from retry import retry
from base import Host, munet_environment
import conftest
import re
import json


class test_basic_environment(munet_environment):
    def __init__(self, name: str):

        super().__init__(name)
       
        if not self.two_hosts_have_connectivity("r3", "r4"):
            raise Exception("r3 can't reach r4")
     
@pytest.fixture(scope="module")
def test_env(request):
    env_name = request.config.getoption("name");
    env = test_basic_environment(env_name)
    
    def show_logs_finalizer():
        print("Finalizer")
        env.show_logs("r3")
        env.teardown_munet()
    request.addfinalizer(show_logs_finalizer)

    return env;

def test_simple(test_env):
    assert 1 == 1

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_learned(test_env):
    rip_routes = test_env.hosts["r3"].execute_rip_cli_r()
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.5.0/24, nh 10.0.3.4, metric 3" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.4.0/24, nh 10.0.3.4, metric 2" in rip_routes)
    assert("ifi 2, dev eth0, rfamily_id 2, rtag 0, network 10.0.1.0/24, nh 10.0.2.2, metric 2" in rip_routes)

@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_static(test_env):
    rip_routes = test_env.hosts["r3"].execute_rip_cli_r()
    assert(len(rip_routes) == 5) # 3 learned, 2 static

    assert("ifi 2, dev eth0, rfamily_id 2, rtag 0, network 10.0.2.0/24, nh 0.0.0.0, metric 1" in rip_routes)
    assert("ifi 3, dev eth1, rfamily_id 2, rtag 0, network 10.0.3.0/24, nh 0.0.0.0, metric 1" in rip_routes)


@retry(AssertionError, tries=5, delay=5.0)
def test_contains_advertised_routes_ip_route(test_env):
    routes = test_env.hosts["r3"].execute_ip_route()
  
    assert("10.0.1.0/24 via 10.0.2.2 dev eth0 proto rip metric 20" in routes)
    assert("10.0.4.0/24 via 10.0.3.4 dev eth1 proto rip metric 20" in routes)
    assert("10.0.5.0/24 via 10.0.3.4 dev eth1 proto rip metric 20" in routes)
    assert(len(routes) == 7)

def test_connectivity(test_env):
    assert test_env.hosts_have_connectivity()

def test_remove_dead_routes(test_env):
    test_env.hosts["r6"].execute_shell("pkill frr")

def test_no_errors(test_env):
    hostnames = []
    if(test_env.env_name == "self_test"):
        hostnames = test_env.hosts.keys()
    else:
        hostnames = ["r3"]
    
    print("hostnames: {}".format(hostnames))
    for hostname in hostnames:
        filepath = test_env.get_log_filename(hostname)
        with open(filepath) as f:
                for line in f:
                    match = re.search('error', line, re.IGNORECASE);
                    if(match):
                        print("error found: '{}' for hostname {}, showing logs...".format(line, hostname))
                        test_env.show_logs(hostname)
                        pytest.fail()
