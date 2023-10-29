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
  sudo \
  pip \
  valgrind \
  pkg-config

#lib deps
RUN apt-get install -yq \
  libnl-3-dev \
  libnl-route-3-dev

#FRR
RUN apt-get install lsb-release -yq
RUN curl -s https://deb.frrouting.org/frr/keys.gpg | tee /usr/share/keyrings/frrouting.gpg > /dev/null
RUN echo deb '[signed-by=/usr/share/keyrings/frrouting.gpg]' https://deb.frrouting.org/frr $(lsb_release -s -c) frr-stable | tee -a /etc/apt/sources.list.d/frr.list
RUN sudo apt-get update && sudo apt-get install frr frr-pythontools -yq

#Munet
RUN pip install munet

RUN apt-get install cmake -y

WORKDIR /workdir/app
COPY . /workdir/app

RUN pip install pytest

#run tests
RUN cd ./src && mkdir build_debug && \
    cd build_debug && cmake .. -DBUILD_TESTING=ON && \
    make 
RUN cd ./src/build_debug/tests && ctest
RUN cd ./src/build_debug/tests && ctest -T memcheck
  
#install 
RUN cd ./src && mkdir build_release && \
    cd build_release && cmake -DBUILD_TESTING=OFF ..  && \
    make && make install 

CMD ["bash", "tests/run.sh"]
