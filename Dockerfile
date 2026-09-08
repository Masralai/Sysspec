FROM ubuntu:22.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build_context

COPY . .

RUN cmake -B build -S . -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build --config Release && \
    strip build/bin/Sysspec || true

FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    libstdc++6 \
    && rm -rf /var/lib/apt/lists/* \
    && useradd -m -u 1000 sysspec

WORKDIR /home/sysspec

COPY --from=builder /build_context/build/bin/Sysspec ./Sysspec

USER sysspec

ENTRYPOINT [ "./Sysspec" ]