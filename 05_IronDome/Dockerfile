# Use an official Ubuntu as a parent image
FROM ubuntu:latest

# Install necessary packages
RUN apt-get update && \
    apt-get install -y build-essential && \
    apt-get install -y nano && \
    apt-get install -y gcc && \
    apt-get install -y make

# Add the irondome source code to /usr/src
ADD irondome.c /usr/src/irondome.c

# Compile the irondome program
RUN gcc -o /usr/local/bin/irondome /usr/src/irondome.c

# Run the irondome program
CMD ["/usr/local/bin/irondome", "/path/to/monitor"]