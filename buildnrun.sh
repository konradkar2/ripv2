#/bin/bash

name=ripv2_container

docker kill $name
docker rm $name
docker build -t ripv2 -f docker/Dockerfile .
docker run --name $name -d ripv2 

sleep 5
docker logs $name

