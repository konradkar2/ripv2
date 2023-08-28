#!/usr/bin/python

from mininet.net import Mininet
from mininet.topo import Topo
from mininet.cli import CLI
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.log import setLogLevel


class SimpleTopology(Topo):
    def build(self):
        # Add two hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        
        # Add a switch
        s1 = self.addSwitch('s1')
        
        # Add links
        self.addLink(h1, s1)
        self.addLink(h2, s1)

if __name__ == '__main__':
    print("Main\n")
    setLogLevel('info')
    
    topo = SimpleTopology()
    
    # Create Mininet network using the custom topology
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink)
    
    # Start the network
    net.start()
    
    # Configure IP addresses for hosts
    h1, h2 = net.get('h1', 'h2')
    h1.cmd('ifconfig h1-eth0 10.0.0.1 netmask 255.255.255.0')
    h2.cmd('ifconfig h2-eth0 10.0.0.2 netmask 255.255.255.0')
    
    # Start a simple web server on h1
    h1.cmd('python -m SimpleHTTPServer 80 &')
    
    # Open an xterm for h2
    h2.cmd('xterm &')
    
    # Run the Mininet command-line interface
    CLI(net)
    
    # Clean up when done
    net.stop()