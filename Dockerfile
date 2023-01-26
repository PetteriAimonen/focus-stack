FROM ubuntu:latest

ENV TZ=Asia/Jerusalem
RUN ln -snf /usr/share/zoneinfo/$TZ /etc/localtime && echo $TZ > /etc/timezone

RUN apt-get update && apt-get install -y \
    tzdata git build-essential libopencv-dev && \
    rm -rf /var/lib/apt/lists/*

RUN git clone --depth=1 https://github.com/PetteriAimonen/focus-stack.git

RUN make -C focus-stack

RUN make install -C focus-stack

CMD ["focus-stack"]
