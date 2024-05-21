# ============ BUILD ============ #
FROM gcc:11.3 as build

RUN apt update && \
    apt install -y \
    python3-pip \
    cmake \
    catch2 \
    && \
    pip3 install conan==1.*

COPY conanfile.txt /app/
RUN mkdir /app/build && cd /app/build && \
    conan profile new default --detect && \
    conan profile update settings.compiler.libcxx=libstdc++11 default && \
    conan install .. --build=missing

COPY ./src /app/src
COPY ./include /app/include
COPY ./tests /app/tests
COPY CMakeLists.txt /app/

RUN cd /app/build && \
    cmake -DCMAKE_BUILD_TYPE=Release .. && \
    cmake --build . 

# ============ RUN ============ #
FROM ubuntu:22.04 as run

RUN groupadd -r alex && useradd -r -g alex alex
USER alex

COPY --from=build /app/build/bin/game_server /app/
COPY ./data /app/data
COPY ./static /app/static

ENTRYPOINT ["/app/game_server", "/app/data/config.json", "/app/static"]