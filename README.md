# RIPv2

## Overview
This project is a RIPv2 implementation based on RFC 2453. It provides a dynamic routing solution for small to medium-sized networks, focusing on essential features outlined in the RFC.

## Key Features
- **RFC 2453 Compliance:** Adheres to the standards specified in [RFC 2453](https://datatracker.ietf.org/doc/rfc2453/).
- **Easy Integration:** Simple and easy to launch.
- **Community Contributions:** Welcomes collaboration and contributions from the community.

## Limitations
- **Partial Feature Set:** Not every functionality is implemented

## Apps
### ripd
The main app
- takes part in the RIPv2 process
- manipulates the routing table
### rip-cli
The CLI app communicates with the ripd via IPC.
The goal is to have the possibility to change the configuration in runtime,
and to have access to the internal state of the program, for testing purposes,

## Dependencies
Ensure the following dependencies are installed:
- [libnl3](https://github.com/thom311/libnl)
- [libyaml](https://github.com/yaml/libyaml)

## Getting started
### Production
To build the project do following
```
cd src && mkdir build && cd build
cmake .. && make install
```
To run it, simply put the config.yaml into /etc/rip/ directory
#### Example Configuration
```yaml
rip_configuration:
  version: 2
  rip_interfaces: 
    - dev: eth0
    - dev: eth1
  advertised_networks: 
    - address: "10.0.2.3"
      prefix: 24
      dev: eth0
    - address: "10.0.3.3"
      prefix: 24
      dev: eth1
```

### Unit testing
To build and run the test do following
```
cd src && mkdir build_testing && cd build_testing
cmake -DBUILD_TESTING=ON .. && make && cd tests
sudo ctest -T memcheck
```
### Integration tests
To run the integration tests run buildnrun.sh script. This will build a docker image, and launch unit/integration tests.
The tests are done against [FRR](https://github.com/FRRouting/frr) and against itself. The network environment is done via [munet](https://github.com/LabNConsulting/munet)

## License
The project is released under an [MIT license](https://github.com/konradkar2/ripv2/blob/main/LICENCE).
