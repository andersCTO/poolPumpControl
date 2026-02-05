#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${SCRIPT_DIR}/build"

echo "=== Pool Pump Controller Host Tests ==="

# Clean and create build directory
rm -rf "${BUILD_DIR}"
mkdir -p "${BUILD_DIR}"

# Configure and build
cd "${BUILD_DIR}"
cmake ..
make -j"$(nproc 2>/dev/null || echo 2)"

echo ""
echo "=== Running tests ==="
ctest --output-on-failure

echo ""
echo "=== All tests passed ==="
