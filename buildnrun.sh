#/bin/bash


name=ripv2_container

docker kill $name
docker rm $name

set -e
docker build -t ripv2 -f docker/Dockerfile .
docker run -it --privileged -e DISPLAY -v /lib/modules:/lib/modules --name $name -d ripv2
           
sleep 5
docker logs ripv2_container  -f


