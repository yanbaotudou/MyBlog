ARG UBUNTU_IMAGE=ubuntu:24.04
FROM ${UBUNTU_IMAGE}

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update \
  && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    pkg-config \
    libdrogon-dev \
    libjsoncpp-dev \
    uuid-dev \
    libpq-dev \
    libmariadb-dev \
    libmariadb-dev-compat \
    libbrotli-dev \
    libhiredis-dev \
    libyaml-cpp-dev \
    zlib1g-dev \
    libargon2-dev \
    libsqlite3-dev \
    libssl-dev \
    ca-certificates \
  && ln -sf /usr/lib/x86_64-linux-gnu/libmariadb.so /usr/lib/x86_64-linux-gnu/libmariadbclient.so \
  && mkdir -p /usr/lib/mariadb \
  && ln -sf /usr/lib/x86_64-linux-gnu/libmariadbclient.so /usr/lib/mariadb/libmariadbclient.so \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend
COPY backend/ /app/backend/
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build -j"$(nproc)"

EXPOSE 8080
CMD ["/app/backend/build/blog_api"]
