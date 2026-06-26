#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
build_dir="${repo_root}/build/rpi"

cmake -S "${repo_root}" -B "${build_dir}" -DCMAKE_BUILD_TYPE=Release
cmake --build "${build_dir}" -j"$(nproc)"

echo
echo "Raspberry Pi build complete."
echo "Headless run example:"
echo "bash scripts/run_rpi.sh \"rtsp://user:password@192.168.1.50:554/stream1\""
