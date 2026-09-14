FROM debian:trixie-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    git \
    libgtk-4-dev \
    libxml2-dev \
    libminizip-dev \
    libgeoip-dev \
    libreadline-dev \
    zlib1g-dev \
    gettext \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /src
