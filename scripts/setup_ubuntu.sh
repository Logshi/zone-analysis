#!/usr/bin/env bash
set -euo pipefail

if [[ "${EUID}" -ne 0 ]]; then
  echo "Bu script apt paketleri kurar. Lutfen sudo ile calistirin:"
  echo "sudo bash scripts/setup_ubuntu.sh"
  exit 1
fi

apt-get update
apt-get install -y build-essential cmake pkg-config libopencv-dev ffmpeg

echo
echo "Kurulum tamamlandi."
echo "Derlemek icin: bash scripts/build_ubuntu.sh"
