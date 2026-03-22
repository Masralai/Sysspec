FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_context

COPY . .

RUN mkdir build && cd build && \
    cmake .. && \
    make

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apy/lists/*

WORKDIR /root

COPY --from=builder /build_context/build/bin/Sysspec .

ENTRYPOINT [ "./Sysspec" ]