FROM drogonframework/drogon:latest

RUN apt-get update \
  && apt-get install -y --no-install-recommends libargon2-dev libsqlite3-dev \
  && rm -rf /var/lib/apt/lists/*

WORKDIR /app/backend
COPY backend/ /app/backend/
RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  && cmake --build build -j"$(nproc)"

EXPOSE 8080
CMD ["/app/backend/build/blog_api"]
