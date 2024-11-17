#!/usr/bin/python
import time
import pytest
from retry import retry
from base import Host, munet_environment
import re
import json
import sys

class ConfParser:
    def __init__(self, cfg_file_name):
        with open(cfg_file_name) as f:
            self.cfg = json.load(f)

    def create_test_metadata(self):
        tests = []
        test_name = self.cfg["name"]
        steps = self.cfg["steps"]
        for step_cfg in steps:
            strategy = step_cfg["strategy"]
            for action in strategy["actions"]:
                test = {"name" : test_name, "host": strategy["host"], "action": action, "do" : action["do"]}
                tests.append(test)
        return tests

class TestExecutor(munet_environment):
    def __init__(self, name: str):
        super().__init__(name)

    def dispatch_test(self, test_metadata):
        hostname = test_metadata["host"]
        host = self.hosts[hostname]

        action_cfg = test_metadata["action"]
        action_type = action_cfg["type"]

        if action_type == "framework_cmd":
            framework_cmd = action_cfg
            data = action_cfg["data"]
            self.distpach_cmd(host, framework_cmd, data)
        elif action_type == "shell_cmd":
            print("shell cmd unimpmented")
        else:
            raise Exception("Invalid action type {}".format(action_type))
    
    def distpach_cmd(self, host, cmd, data):
        if cmd == "expect_rip_cli_routing_len":
            entries = host.execute_rip_cli_r()
            assert(len(entries) == data)
        elif cmd == "expect_rip_cli_routing_contains":
            print("expect_rip_cli_routing_contains unimplemnted")


def generate_tests():
    parser = ConfParser("test-cfg.json")
    tests_metadata = parser.create_test_metadata()
    for test_metadata in tests_metadata:
        test_name = "test_{}_{}_{}".format(test_metadata["name"], test_metadata["host"], test_metadata["do"] )
        def template_test(environment):
            environment.dispatch_test(test_metadata)
        
        template_test.__name__ = test_name
        globals()[test_name] = template_test  # Add to the global namespace

# Generate and register tests
generate_tests()

@pytest.fixture(scope="module")
def create_test_executor():
    print("\n[SETUP] Module-level shared resource")

    name = ""
    with open("test-cfg.json") as f:
        d = json.load(f)
        name = d["name"]

    testExecutor = TestExecutor(name)
    yield testExecutor
    print("\n[TEARDOWN] Module-level shared resource")

# Main function to run the tests
def main():
    pytest.main(["-v", __file__])

if __name__ == "__main__":
    main()




        




