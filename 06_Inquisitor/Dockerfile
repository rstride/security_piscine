FROM debian:latest

RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y nano && \
    apt-get install -y gcc && \
    apt-get install -y make \
    apt-get install -y libcap-dev

COPY . /app
WORKDIR /app

RUN make

CMD ["sleep", "infinity"]