ARG UBUNTU_TAG=focal
FROM ubuntu:${UBUNTU_TAG} as ubuntu
ENV DEBIAN_FRONTEND noninteractive

FROM ubuntu as basics
RUN apt-get update && \
    apt-get install -y \
    g++ \
    make \
    cmake \
    git \
    curl \
    pkg-config \
    uuid-dev \
    libboost1.71-dev

FROM basics as arrow
RUN apt-get update
RUN apt-get install -y -V ca-certificates lsb-release wget && \
    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get update && \
    apt-get install -y -V libarrow-dev

COPY jsontest/cpp /src

ENTRYPOINT ["/bin/bash"]
