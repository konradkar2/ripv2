#/bin/bash


name=ripv2_container

docker kill $name
docker rm $name

set -e
docker build -t ripv2 -f docker/Dockerfile .
docker run -it --privileged -e DISPLAY -v /tmp/ripv2_tests_docker/:/tmp/ripv2_tests/ --name $name -d ripv2
           
sleep 5
docker logs ripv2_container  -f


