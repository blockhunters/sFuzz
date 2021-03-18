FROM gcc

RUN apt-get update && apt-get -y install libleveldb-dev

COPY . /app
WORKDIR /app

RUN /app/scripts/install_cmake.sh

RUN mkdir build            # Create a build directory.
RUN cd build && /root/.local/bin/cmake .. && \
    cd fuzzer && make        # Build fuzzer targets.
