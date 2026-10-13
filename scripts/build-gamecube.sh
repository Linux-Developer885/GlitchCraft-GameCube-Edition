#!/bin/sh
set -eu

repository=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
image="devkitpro/devkitppc@sha256:44cb1a920e1ec3ec7c06767493c3b85f8d643d6137cc4661f0201895ac6e4967"

if ! command -v docker >/dev/null 2>&1; then
    echo "Docker is required; install devkitPro gamecube-dev to build natively." >&2
    exit 1
fi

if ! docker image inspect "$image" >/dev/null 2>&1; then
    echo "The pinned devkitPro toolchain image is not installed." >&2
    echo "Run: docker pull $image" >&2
    exit 1
fi

exec docker run --rm \
    --user "$(id -u):$(id -g)" \
    --env HOME=/tmp \
    --volume "$repository:/work" \
    --workdir /work \
    "$image" make -f Makefile.gamecube "$@"
