FROM ubuntu:18.04

MAINTAINER Ray Burgemeestre

RUN apt-get update && apt-get -y install g++ git cmake sudo wget

RUN g++ -v

#RUN git submodule update --init --recursive
#
#RUN apt-get install -y gnupg2
#
#RUN echo deb http://apt.llvm.org/bionic/ llvm-toolchain-bionic-7 main >> /etc/apt/sources.list && \
#    echo deb-src http://apt.llvm.org/bionic/ llvm-toolchain-bionic-7 main >> /etc/apt/sources.list && \
#    wget -O - https://apt.llvm.org/llvm-snapshot.gpg.key|sudo apt-key add - ; \
#    apt update -y && \
#    apt-get install -y clang-7 lldb-7 lld-7 && \
#    apt-get install -y libc++-7-dev libc++-7-dev

WORKDIR /usr/local/src/framer

CMD "/bin/bash"
