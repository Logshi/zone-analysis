#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
  echo "Usage: bash scripts/run_ubuntu.sh <rtsp_url|video_path|webcam_index> [dwell_seconds] [region]"
  exit 1
fi

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_input="$1"
dwell_seconds="${2:-10}"
region="${3:-180,200;500,200;560,560;120,560}"
exe="${repo_root}/build/unix/dwell_alert"
model="${repo_root}/models/yolov8n.onnx"
alerts="${repo_root}/alerts"

if [[ ! -x "${exe}" ]]; then
  echo "Executable not found. Build it first:"
  echo "bash scripts/build_ubuntu.sh"
  exit 1
fi

"${exe}" "${source_input}" "${model}" --dwell "${dwell_seconds}" --region "${region}" --alerts "${alerts}"
