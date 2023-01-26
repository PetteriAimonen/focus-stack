FROM ubuntu:20.04

ENV TZ=Asia/Jerusalem
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y --no-install-recommends \
            tzdata git build-essential cmake pkg-config wget unzip libgtk2.0-dev \
            curl ca-certificates libcurl4-openssl-dev libssl-dev \
            libavcodec-dev libavformat-dev libswscale-dev libtbb2 libtbb-dev \
            libjpeg-turbo8-dev libpng-dev libtiff-dev libdc1394-22-dev nasm libopencv-dev && \
            rm -rf /var/lib/apt/lists/*

ADD https://github.com/PetteriAimonen/focus-stack/releases/download/1.4/focus-stack_1.3-10-g8649bc7_amd64.deb /usr/src
RUN dpkg -i /usr/src/focus-stack_1.3-10-g8649bc7_amd64.deb
CMD ["focus-stack"]
