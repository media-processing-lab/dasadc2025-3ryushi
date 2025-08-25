FROM ubuntu:20.04
# Install AArch64 cross-compilation tools
RUN apt-get update && \
    apt-get install -y \
    gcc-aarch64-linux-gnu \
    g++-aarch64-linux-gnu \
    libc6-dev-arm64-cross \
    binutils-aarch64-linux-gnu \
    binutils

CMD ["/bin/bash"]

