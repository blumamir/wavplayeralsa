 
FROM ubuntu:18.04 AS build

# RUN echo "http://dl-cdn.alpinelinux.org/alpine/edge/testing" >> /etc/apk/repositories

ARG DEBIAN_FRONTEND="noninteractive"

RUN apt-get update && \
    apt-get -y install \
        tzdata \
        build-essential \
        cmake \
        uuid-dev \
        libsndfile1-dev \
        libasound2-dev \
        libboost-all-dev

COPY ./src /src/src
COPY ./thirdparty /src/thirdparty
COPY ./CMakeLists.txt /src

RUN cd /src && \
    cmake . && make


FROM ubuntu:18.04

WORKDIR /opt/wavplayeralsa

COPY --from=build /src/wavplayeralsa ./

RUN apt-get update && \
    apt-get install -y \
        libsndfile1 \
        libasound2 \ 
        libboost-system-dev \ 
        libboost-filesystem-dev

CMD ["./wavplayeralsa", "-d", "/wav_files", "--http_listen_port", "80"]