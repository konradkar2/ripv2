#/bin/bash

name=ripv2_container

docker kill $name
docker rm $name
docker build -t ripv2 -f docker/Dockerfile .
docker run --name $name -d ripv2 
docker logs -f $name

