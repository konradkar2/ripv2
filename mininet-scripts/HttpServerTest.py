#!/usr/bin/python

from mininet.net import Mininet
from mininet.topo import Topo
from mininet.cli import CLI
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from mininet.log import setLogLevel
from mininet.log import lg, info, error

class SimpleTopology(Topo):
    def build(self):
        print("SimpleTopologyBuild")
        # Add two hosts
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')
        
        # Add a switch
        s1 = self.addSwitch('s1')
        
        # Add links
        self.addLink(h1, s1)
        self.addLink(h2, s1)

if __name__ == '__main__':
    setLogLevel('debug')

    print("Main\n")     
    topo = SimpleTopology()
    
    # Create Mininet network using the custom topology
    net = Mininet(topo=topo, host=CPULimitedHost, link=TCLink)
    
    # Start the network
    net.start()
    
    print("Configure IP addresses for hosts")

    h1, h2 = net.get('h1', 'h2')
    h1.cmd('ip address add 10.0.0.1/24 dev h1-eth0')
    h2.cmd('ip address add 10.0.0.2/24 dev h1-eth0')
    
    print("Run http server")
    h1.cmd('python -m SimpleHTTPServer 80 &')
    
    # Open an xterm for h2
    print("Open xterm")
    h2.cmd('xterm &')
    
    print("Run CLI")
    # Run the Mininet command-line interface
    CLI(net)
    
    # Clean up when done
    print("Net stop")
    net.stop()

    print("done")