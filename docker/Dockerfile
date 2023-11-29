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
  pkg-config \ 
  git \
  cmake 

#lib deps
RUN apt-get install -yq \
  libnl-3-dev \
  libnl-route-3-dev

RUN git clone --depth 1 --branch 0.2.5 https://github.com/yaml/libyaml.git
RUN mkdir libyaml/build && cd libyaml/build && cmake .. && make install


#FRR
RUN apt-get install lsb-release -yq
RUN curl -s https://deb.frrouting.org/frr/keys.gpg | tee /usr/share/keyrings/frrouting.gpg > /dev/null
RUN echo deb '[signed-by=/usr/share/keyrings/frrouting.gpg]' https://deb.frrouting.org/frr $(lsb_release -s -c) frr-stable | tee -a /etc/apt/sources.list.d/frr.list
RUN sudo apt-get update && sudo apt-get install frr frr-pythontools -yq

#Munet
RUN pip install munet 

RUN pip install pytest retry

WORKDIR /workdir/app
COPY . /workdir/app

#run tests
RUN cd ./src && mkdir build_debug && \
    cd build_debug && cmake .. -DBUILD_TESTING=ON && \
    make 
  
#install release
RUN cd ./src && mkdir build_release && \
    cd build_release && cmake -DBUILD_TESTING=OFF ..  && \
    make && make install 

WORKDIR /workdir/app
CMD ["bash", "tests/run.sh"]
