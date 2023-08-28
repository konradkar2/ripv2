FROM ubuntu:22.04
RUN echo 'Run TestMininet'
RUN apt-get update && apt-get install -yq \
  vim \
  iproute2 \
  iputils-ping \
  build-essential \
  python3 \
  mininet \
  openvswitch-testcontroller \
  tree

WORKDIR /app
COPY . /app

CMD ["python3", "mininet-scripts/HttpServerTest.py"]
