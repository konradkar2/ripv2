#!/usr/bin/python
import time
import pytest
from retry import retry
from base import Host, MunetEnvironment
import re
import json
import sys
import time
from pprint import pformat

class TestMetadata:
    __test__ = False
    def __init__(self, name, strategy, action, action_number):
        self.name = name
        self.strategy = strategy
        self.action = action
        self.action_number = action_number
        self.host_name = strategy["host"]

    def unique_test_name(self):
        return "test_{}_{}_{}_{}".format(self.name, self.host_name, self.action["do"], self.action_number)

    def namespace_name(self):
        return self.name
    
    def retries(self):
        return self.action.get("retries", 1)
    
    def retry_period(self):
        return self.action.get("retry_period", 1)

    def __str__(self):
        return f"{self.__class__.__name__}:\n{pformat(vars(self))}"

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
            actionIdx = 0
            for action in strategy["actions"]:
                tests.append(TestMetadata(test_name, strategy, action, actionIdx))
                actionIdx = actionIdx + 1
        
        return tests


class TestExecutor():
    __test__ = False

    def __init__(self, munet_environment):
        self.munet_env = munet_environment

    def dispatch_test(self, test_metadata):
        host = self.munet_env.hosts[test_metadata.host_name]

        action_cfg = test_metadata.action
        action_type = action_cfg["type"]

        if action_type == "framework_cmd":
            framework_cmd = action_cfg
            self.distpach_framwork_action(host, action_cfg)
        elif action_type == "shell_cmd":
            print("shell cmd unimpmented")
        else:
            raise Exception("Invalid action type {}".format(action_type))
    
    def distpach_framwork_action(self, host, action_cfg):
        data = action_cfg["data"]
        do = action_cfg["do"]

        if do == "expect_rip_cli_routing_len":
            entries = host.execute_rip_cli_r()
            assert(len(entries) == data)
        elif do == "expect_rip_cli_routing_contains":
            entries = host.execute_rip_cli_r()
            for expected_entry in data:
                assert(expected_entry in entries)
        else: 
            raise Exception("invalid framework do: {}".format(do))


def generate_tests():
    parser = ConfParser(sys.argv[1])
    tests_metadata = parser.create_test_metadata()
    for test_metadata in tests_metadata:
        
        test_name = test_metadata.unique_test_name()
        def template_test(test_metadata=test_metadata, test_name=test_name):
            print(f"Running test: {test_name}")
            namespace_name = test_metadata.namespace_name()
            retries = test_metadata.retries()
            retry_period = test_metadata.retry_period()

            env = get_munet_environment(namespace_name, parser.cfg["hostIdToIpAddress"])
            testExecutor = TestExecutor(env) 

            last_exception = None
            for attempt in range(1, retries + 1):
                try:
                    testExecutor.dispatch_test(test_metadata)
                    return  # Test passed; exit function
                except AssertionError as e:
                    last_exception = e
                    print(f"[Retry {attempt}/{retries}] Test '{test_name}' failed: {e}")
                    if attempt != retries:
                        time.sleep(retry_period)
            
            # If all retries fail, raise the last captured exception
            if last_exception:
                raise AssertionError(f"Test '{test_name}' failed after {retries} retries") from last_exception
        
        template_test.__name__ = test_name
        globals()[test_name] = template_test  # Add to the global namespace

# Generate and register tests
generate_tests()

_munet_env_instance = None
def get_munet_environment(name, hostIdToIpAddress):
    global _munet_env_instance
    if _munet_env_instance is None:
        if name is None:
            raise ValueError("Must provide a name for the initial environment creation.")
        print(f"[Singleton] Initializing MunetEnvironment with name: {name}")
        _munet_env_instance = MunetEnvironment(name, hostIdToIpAddress)
    return _munet_env_instance

# Main function to run the tests
def main():
    if len(sys.argv) != 2:
        raise Exception("Invalid number of arguments")

    pytest_args = [
        "-v",         # Verbose output
        "-s",         # Disable stdout capturing
        __file__      # Run the current script's tests
    ]
    pytest.main(pytest_args)

if __name__ == "__main__":
    main()




        




