# ARG UBUNTU_TAG=focal
# FROM ubuntu:${UBUNTU_TAG} as ubuntu
FROM rapidsai/rapidsai-core-dev-nightly:21.08-cuda11.2-devel-ubuntu20.04-py3.8 as ubuntu

ENV DEBIAN_FRONTEND noninteractive

FROM ubuntu as basics
RUN apt-get update && \
    apt-get install -y \
    g++ \
    make \
    git \
    curl \
    pkg-config \
    uuid-dev \
    libboost1.71-dev

RUN curl -L https://github.com/Kitware/CMake/releases/download/v3.20.5/cmake-3.20.5-linux-x86_64.tar.gz | tar xz --strip-components=1 -C /usr

FROM basics as antlr4
RUN apt-get install -y antlr4 && \
    cd /usr/local/lib && \
    curl -O https://www.antlr.org/download/antlr-4.9-complete.jar
ENV CLASSPATH=".:/usr/local/lib/antlr-4.9-complete.jar:$CLASSPATH"

FROM antlr4 as arrow
RUN apt-get install -y -V ca-certificates lsb-release wget && \
    wget https://apache.jfrog.io/artifactory/arrow/$(lsb_release --id --short | tr 'A-Z' 'a-z')/apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get install -y -V ./apache-arrow-apt-source-latest-$(lsb_release --codename --short).deb && \
    apt-get update && \
    apt-get install -y -V libarrow-dev

COPY jsontest/cpp /src

ENTRYPOINT ["/bin/bash"]
