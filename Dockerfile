# ==========================================
# Stage 1: Build Environment (Linux Compiler)
# ==========================================
FROM ubuntu:24.04 AS builder

ENV DEBIAN_FRONTEND=noninteractive

# Install g++, CMake, and Ninja
RUN apt-get update && apt-get install -y \
    g++ \
    cmake \
    ninja-build \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace

# Copy everything from your Windows folder into the container
COPY . .

# Run CMake and build the database
RUN cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build

# ==========================================
# Stage 2: Minimal Runtime (Production-grade)
# ==========================================
FROM ubuntu:24.04

WORKDIR /app

# Copy ONLY the compiled Linux binary away from the compiler layers
COPY --from=builder /workspace/build/KV_Database .

EXPOSE 6625

VOLUME ["/app/data"]

CMD ["./KV_Database"]