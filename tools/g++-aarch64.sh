#!/bin/bash -eu
SCRIPT_DIR="$(cd -P -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd -P)"

# docker-compose or docker compose
if ! command -v docker compose &> /dev/null; then
    if command -v docker-compose &> /dev/null; then
        alias docker compose='docker-compose'
    else
        echo "Error: docker compose or docker-compose command not found."
        exit 1
    fi
fi

PWD="${PWD}" FLAGS="$@" \
    docker compose -f "$SCRIPT_DIR/compiler_container/docker-compose.yml" run --rm --build gcc
if [ $? -eq 0 ]; then
    echo "Compiled successfully."
fi