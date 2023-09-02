FROM ubuntu:22.04
RUN echo 'Run TestMininet'
RUN mkdir -p /var/run/sshd

RUN apt-get update && apt-get install -yq \
  vim \
  curl \
  dnsutils \
  ifupdown \
  iproute2 \
  iptables \
  iputils-ping \
  net-tools \
  tcpdump \
  vim \
  x11-xserver-utils \
  xterm \ 
  sudo 

#MININET
RUN apt-get install -yq \
  mininet \
  openvswitch-switch \
  openvswitch-testcontroller 

#FRR
RUN apt-get install lsb-release -yq
RUN curl -s https://deb.frrouting.org/frr/keys.gpg | tee /usr/share/keyrings/frrouting.gpg > /dev/null
RUN echo deb '[signed-by=/usr/share/keyrings/frrouting.gpg]' https://deb.frrouting.org/frr $(lsb_release -s -c) frr-stable | tee -a /etc/apt/sources.list.d/frr.list
RUN sudo apt-get update && sudo apt-get install frr frr-pythontools -yq



WORKDIR /workdir/app
COPY . /workdir/app
CMD ["bash", "mininet-scripts/run.sh"]
