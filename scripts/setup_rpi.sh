#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "This script installs apt packages. Please run it with sudo:"
  echo "sudo bash scripts/setup_rpi.sh"
  exit 1
fi

apt-get update
apt-get install -y \
  build-essential \
  cmake \
  git \
  pkg-config \
  libopencv-dev \
  ffmpeg

echo
echo "Raspberry Pi setup complete."
echo "To build: bash scripts/build_rpi.sh"
