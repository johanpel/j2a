ARG UBUNTU_TAG=focal
FROM ubuntu:${UBUNTU_TAG} as ubuntu
ENV DEBIAN_FRONTEND noninteractive

FROM ubuntu as build
RUN apt-get update && \
    apt-get install -y \
    g++ \
    make \
    cmake \
    git \
    antlr4
RUN apt install -y -V ca-certificates lsb-release wget && \
    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt update && \
    apt install -y -V libarrow-dev

ENTRYPOINT ["/bin/bash"]
