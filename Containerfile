# Containerfile for building Braids firmware
FROM python:2.7-slim

# Switch to archive repos for old Debian Buster
RUN sed -i 's|http://deb.debian.org/debian|http://archive.debian.org/debian|g' /etc/apt/sources.list && \
    sed -i 's|http://security.debian.org/debian-security|http://archive.debian.org/debian-security|g' /etc/apt/sources.list && \
    echo 'Acquire::Check-Valid-Until "false";' > /etc/apt/apt.conf.d/99disable-valid-until

# Install essential build tools and ARM cross-compiler
RUN apt-get update && \
    apt-get install -y build-essential gcc-arm-none-eabi git curl && \
    rm -rf /var/lib/apt/lists/*

# Install numpy into /tmp/python2-packages
RUN pip install --prefix=/tmp/python2-packages numpy

# Set PYTHONPATH so Python 2 sees numpy
ENV PYTHONPATH=/tmp/python2-packages/lib/python2.7/site-packages:$PYTHONPATH

# Set working directory inside container
WORKDIR /mi
