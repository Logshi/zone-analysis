#!/usr/bin/env bash
set -euo pipefail

# Builds and installs OpenCV from source on Raspberry Pi.
#
# Why this is needed: the OpenCV version shipped via apt on Raspberry Pi OS
# bookworm (4.6.0) fails to run YOLOv8's anchor-free detection head through
# cv::dnn::Net::forward() (a shape_utils.hpp assertion). OpenCV 4.10+ does
# not have this problem. This script builds OpenCV 4.10.0 from source and
# installs it to /usr/local, which CMake's find_package(OpenCV) picks up
# automatically ahead of the apt-installed copy in /usr.
#
# This takes a long time on Raspberry Pi hardware (roughly 1-3+ hours
# depending on the model) and needs a few GB of free disk space.

OPENCV_VERSION="4.10.0"
SRC_DIR="${HOME}/opencv-build"
JOBS="${JOBS:-$(nproc)}"

if [[ "${EUID}" -eq 0 ]]; then
  echo "Run this script as a normal user (it will use sudo only where needed):"
  echo "bash scripts/build_opencv_rpi.sh"
  exit 1
fi

echo "[build_opencv_rpi] Installing build dependencies..."
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake git pkg-config \
  libjpeg-dev libpng-dev libtiff-dev \
  libavcodec-dev libavformat-dev libswscale-dev libv4l-dev \
  libxvidcore-dev libx264-dev \
  libatlas-base-dev gfortran \
  python3-dev

mkdir -p "${SRC_DIR}"
cd "${SRC_DIR}"

if [[ ! -d opencv ]]; then
  echo "[build_opencv_rpi] Cloning OpenCV ${OPENCV_VERSION}..."
  git clone --branch "${OPENCV_VERSION}" --depth 1 https://github.com/opencv/opencv.git
fi

cd opencv
mkdir -p build
cd build

echo "[build_opencv_rpi] Configuring (this can take a few minutes)..."
cmake \
  -DCMAKE_BUILD_TYPE=RELEASE \
  -DCMAKE_INSTALL_PREFIX=/usr/local \
  -DBUILD_EXAMPLES=OFF \
  -DBUILD_TESTS=OFF \
  -DBUILD_PERF_TESTS=OFF \
  -DBUILD_opencv_python2=OFF \
  -DBUILD_opencv_python3=OFF \
  -DINSTALL_PYTHON_EXAMPLES=OFF \
  -DINSTALL_C_EXAMPLES=OFF \
  -DWITH_FFMPEG=ON \
  -DBUILD_opencv_dnn=ON \
  -DOPENCV_GENERATE_PKGCONFIG=ON \
  ..

echo "[build_opencv_rpi] Building with ${JOBS} job(s). This is the slow part..."
make -j"${JOBS}"

echo "[build_opencv_rpi] Installing to /usr/local..."
sudo make install
sudo ldconfig

echo
echo "OpenCV ${OPENCV_VERSION} installed to /usr/local."
echo "Rebuild the app with a clean build dir so CMake re-detects OpenCV:"
echo "rm -rf build/rpi && bash scripts/build_rpi.sh"
