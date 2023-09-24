#!/usr/bin/python
from time import sleep
import pexpect


def wait_for_prompt(munet, timeout=5):
    munet.expect("munet>", timeout)
    return munet


def host_execute_cmd(munet, host: str, command: str, timeout=5) -> str:
    munet.sendline("{host} sh {cmd}".format(host=host, cmd=command))
    wait_for_prompt(munet, timeout)
    return munet.before.decode()


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
            "{0} failed: '{1}' not found in '{2}'".format(msg, substring, string)
        )


if __name__ == "__main__":
    munet_ns_dir = "/tmp/test_ns"
    munet = run_munet(munet_ns_dir)
    sleep(3)

    r2_ip_a_result = host_execute_cmd(munet, "r2", "ip a")
    print(r2_ip_a_result)

    frr_ping_stdout = host_execute_cmd(munet, "r2", "ping 10.0.3.4 -c 5")
    verify_substring_exist(
        frr_ping_stdout, "64 bytes from 10.0.3.4", "FRR converged(r2 reaches r4)"
    )
