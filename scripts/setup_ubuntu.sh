#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "This script installs apt packages. Please run it with sudo:"
  echo "sudo bash scripts/setup_ubuntu.sh"
  exit 1
fi

apt-get update
apt-get install -y build-essential cmake pkg-config libopencv-dev ffmpeg

echo
echo "Setup complete."
echo "To build: bash scripts/build_ubuntu.sh"
