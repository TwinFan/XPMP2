# Linux and MacOS compile environment, suitable for compiling X-Plane plugins
FROM ubuntu:18.04

# set up package manager with latest mirrors and certificates
RUN sed -i -e 's/http:\/\/archive.ubuntu.com\/ubuntu\//mirror:\/\/mirrors.ubuntu.com\/mirrors.txt/' /etc/apt/sources.list
RUN apt-get update
RUN apt-get install -y --no-install-recommends ca-certificates

# Install Linux toolchain (GCC) including CMake
RUN apt-get install -y --no-install-recommends build-essential ninja-build sudo bash coreutils curl libcurl4-openssl-dev zlib1g-dev libpng-dev
RUN apt-get clean
RUN curl -sSL https://github.com/Kitware/CMake/releases/download/v3.19.2/cmake-3.19.2-Linux-x86_64.tar.gz | tar -xz -C /usr/local/ --strip-components=1

# Install dependency libraries under Linux.
RUN apt-get install -y --no-install-recommends freeglut3-dev libudev-dev libopenal-dev && apt-get clean

# Install OS X cross-compiling toolchain (clang).
RUN apt-get install -y --no-install-recommends clang curl && apt-get clean
#Build arguments
ARG osxcross_repo="tpoechtrager/osxcross"
ARG osxcross_revision="f4ba4facae996b3b14d89eb62c0384564f7368b5"
ARG darwin_sdk_version="10.11"
ARG darwin_sdk_url="https://github.com/apriorit/osxcross-sdks/raw/master/MacOSX10.11.sdk.tar.xz"

# ENV available in docker image
ENV OSXCROSS_REPO="${osxcross_repo}" \
    OSXCROSS_REVISION="${osxcross_revision}" \
    DARWIN_SDK_VERSION="${darwin_sdk_version}" \
    DARWIN_SDK_URL="${darwin_sdk_url}" \
    CROSSBUILD=1

RUN mkdir -p "/tmp/osxcross" \
 && cd "/tmp/osxcross" \
 && curl -sSLo osxcross.tar.gz "https://codeload.github.com/${OSXCROSS_REPO}/tar.gz/${OSXCROSS_REVISION}" \
 && tar --strip=1 -xzf osxcross.tar.gz \
 && rm -f osxcross.tar.gz \
 && curl -sLo tarballs/MacOSX${DARWIN_SDK_VERSION}.sdk.tar.xz \
             "${DARWIN_SDK_URL}" \
 && UNATTENDED=1 JOBS=4 SDK_VERSION=10.11 ./build.sh \
 && mv target /usr/osxcross \
 && mv tools /usr/osxcross/ \
 && rm -rf "/usr/osxcross/SDK/MacOSX${DARWIN_SDK_VERSION}.sdk/usr/share/man" \
 && true

# Add essential users to the docker image
RUN echo "" | adduser --uid 1000 --disabled-password  --gecos "" xpbuild && adduser xpbuild sudo
RUN echo '%sudo ALL=(ALL) NOPASSWD:ALL' >> /etc/sudoers

VOLUME /src
USER xpbuild
ADD build.sh /usr/bin/build.sh

# Entrypoint is the build.sh script, which takes care of actual building
WORKDIR /Src
ENTRYPOINT ["build.sh"]
