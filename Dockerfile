FROM gcc as builder

RUN apt-get update && apt-get -y install libleveldb-dev

COPY . /source
RUN /source/scripts/install_cmake.sh

RUN mkdir /build            # Create a build directory.
RUN cd /build && /root/.local/bin/cmake /source && \
    cd fuzzer && make        # Build fuzzer targets.



FROM ubuntu

RUN apt-get update && apt-get install -y python3-pip libleveldb-dev
RUN pip3 install solc-select

COPY --from=builder /build/fuzzer/fuzzer /home/
COPY assets /home/assets

WORKDIR /home
