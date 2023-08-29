FROM ubuntu:22.04
RUN echo 'Run TestMininet'
RUN apt-get update && apt-get install -yq \
  vim \
  iproute2 \
  iputils-ping \
  build-essential \
  python3 \
  sudo \
  git \
  tree

WORKDIR /workdir

RUN git clone https://github.com/mininet/mininet /workdir/mininet

WORKDIR /workdir/mininet
#RUN git checkout -b mininet-2.3.0 2.3.0 
RUN util/install.sh -nfv

WORKDIR /workdir/app
COPY . /workdir/app
CMD ["bash", "mininet-scripts/run.sh"]
