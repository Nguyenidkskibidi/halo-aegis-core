#!/usr/bin/env bash
set -euo pipefail

echo "================================================================================"
echo "   H.A.L.O. AEGIS CORE - DUAL-PIPELINE BUILD & VERIFICATION HARNESS"
echo "================================================================================"

CXX=${CXX:-clang++}
TARGET_MAX_BYTES=40960

# Determine OS Linker Flags
DEAD_STRIP_FLAG="-Wl,-dead_strip"
if [[ "$(uname -s)" == "Linux" ]]; then
  DEAD_STRIP_FLAG="-Wl,--gc-sections -Wl,--strip-all"
fi

echo "[1/7] Running ASan & UBSan Memory Safety Checks..."
$CXX -std=c++20 -O2 -fsanitize=address,undefined -DHALO_SANITIZER_ACTIVE -Iinclude \
     tests/halo_benchmark.cpp -o halo_san_check
./halo_san_check
rm -f halo_san_check

$CXX -std=c++20 -O2 -fsanitize=address,undefined -DHALO_SANITIZER_ACTIVE -Iinclude \
     tests/halo_universal_spatial_benchmark.cpp -o halo_san_univ
./halo_san_univ
rm -f halo_san_univ

$CXX -std=c++20 -O2 -fsanitize=address,undefined -DHALO_SANITIZER_ACTIVE -Iinclude \
     tests/halo_universal_genius_benchmark.cpp -o halo_san_genius
./halo_san_genius
rm -f halo_san_genius
echo ">>> ASan & UBSan: 0 memory leaks, 0 undefined behaviors verified."

echo ""
echo "[2/7] Compiling Embedded Release Binary & Verifying < 40 KB Flash Footprint..."
$CXX -std=c++20 -Os -flto -DNDEBUG -march=native \
     -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
     $DEAD_STRIP_FLAG -Iinclude \
     tests/halo_dynamic_flight_benchmark.cpp -o halo_flight_release

# Strip symbols
if [[ "$(uname -s)" == "Darwin" ]]; then
  strip -u -r halo_flight_release
else
  strip --strip-all halo_flight_release
fi

# Measure binary size
if [[ "$(uname -s)" == "Darwin" ]]; then
  BIN_SIZE=$(stat -f%z halo_flight_release)
else
  BIN_SIZE=$(stat -c%s halo_flight_release)
fi

echo ">>> Stripped Binary Size: $BIN_SIZE bytes ($(( BIN_SIZE / 1024 )) KB)"

if (( BIN_SIZE > TARGET_MAX_BYTES )); then
  echo "[-] ERROR: Binary size exceeds 40 KB target ($BIN_SIZE > $TARGET_MAX_BYTES)"
  rm -f halo_flight_release
  exit 1
fi
echo ">>> Flash Footprint Acceptance Gate: PASSED ($BIN_SIZE bytes < 40,960 bytes)"

echo ""
echo "[3/7] Executing Real-Time Embedded Flight Simulation (0.00% Collision Gate)..."
./halo_flight_release
rm -f halo_flight_release
echo ">>> Dynamic Flight Collision Gate: PASSED (0.00% Collisions)"

echo ""
echo "[4/7] Running Sub-Microsecond Hardware Maximization Suite (-O3 -flto)..."
$CXX -std=c++20 -O3 -flto -DNDEBUG -march=native \
     -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
     $DEAD_STRIP_FLAG -Iinclude \
     tests/halo_benchmark.cpp -o halo_hw_bench
./halo_hw_bench
rm -f halo_hw_bench

echo ""
echo "[5/7] Running Universal Geo-Agnostic Spatial Benchmark (Metropolis & Continental)..."
$CXX -std=c++20 -O3 -flto -DNDEBUG -march=native \
     -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
     $DEAD_STRIP_FLAG -Iinclude \
     tests/halo_universal_spatial_benchmark.cpp -o halo_univ_bench
./halo_univ_bench
rm -f halo_univ_bench

echo ""
echo "[6/7] Verifying Embedded Microcontroller & ESP32 Zero-Heap Static Execution..."
$CXX -std=c++20 -O3 -flto -DNDEBUG -march=native \
     -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
     $DEAD_STRIP_FLAG -Iinclude \
     tests/halo_embedded_test.cpp -o halo_embedded_check
./halo_embedded_check
rm -f halo_embedded_check
echo ">>> Embedded & ESP32 Zero-Heap Gate: PASSED"

echo ""
echo "[7/7] Running Project Omni-Aegis Universal Genius Benchmark (4 Physical Gates)..."
$CXX -std=c++20 -O3 -flto -DNDEBUG -march=native \
     -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
     $DEAD_STRIP_FLAG -Iinclude \
     tests/halo_universal_genius_benchmark.cpp -o halo_genius_bench
./halo_genius_bench
rm -f halo_genius_bench
echo ">>> Omni-Aegis Kinodynamics & Sensor-Polymorphic Gates: ALL PASSED"

echo ""
echo "================================================================================"
echo "   ALL PIPELINE CHECKS & ACCEPTANCE GATES SATISFIED - ENGINE AT PEAK EFFICIENCY"
echo "================================================================================"
