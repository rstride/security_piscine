FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        build-essential \
        libpcap-dev \
        iproute2 \
        net-tools \
        iputils-ping \
    && rm -rf /var/lib/apt/lists/*

COPY . /app
WORKDIR /app

RUN make build

CMD ["sleep", "infinity"]
