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
  xterm 

RUN apt-get install -yq \
  mininet \
  openvswitch-switch \
  openvswitch-testcontroller 

RUN apt-get install quagga

WORKDIR /workdir/app
COPY . /workdir/app
CMD ["bash", "mininet-scripts/run.sh"]
