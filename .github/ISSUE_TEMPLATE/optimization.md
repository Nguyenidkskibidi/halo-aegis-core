---
name: "⚡ Performance Optimization (English)"
about: Propose a silicon-level optimization, SIMD trick, or cache alignment speedup.
title: "[PERF]: "
labels: ["performance", "optimization"]
assignees: []
---

## 👤 Submitter Info
- **GitHub Username:** @your-username <!-- Please provide your GitHub handle so we can tag, credit, and contact you easily -->

## 🎯 Target Subsystem & Routine
<!-- Which component are you optimizing? -->
- [ ] SWAR / SIMD Bitboard Raycasting (`halo_simd.h`, `halo_swar_10_layer_bitboard.h`)
- [ ] Jump Point Search+ Micro-Router (`halo_jps_plus.h`)
- [ ] Kinodynamic Quintic Trajectory Solver (`halo_kinodynamics.h`)
- [ ] Sensor-Polymorphic Zero-Copy Ingestion (`halo_sensor_fusion.h`)
- [ ] Embedded Fixed32 Trigonometric / Sqrt Engine (`halo_fixed_point.h`)
- [ ] Memory Layout / Cache Line Alignment / Zero-Allocation Guarantees (`halo_memory.h`)
- [ ] Other: <!-- Specify here -->

## ⏱️ Current Baseline Telemetry
<!-- Paste output from `./scripts/build_and_verify.sh` or Google Benchmark `tests/halo_google_benchmark.cpp` -->
- **Target Function / Benchmark:** <!-- e.g., BM_SWAR_RaycastRow or BM_Kinodynamics_PurePursuit -->
- **Current Latency:** <!-- e.g., 0.35 ns/ray, or 25.5 ns/tick -->
- **Current Throughput:** <!-- e.g., 2.31 G ops/s -->
- **Hardware Tested On:** <!-- e.g., Apple Silicon M3 Pro, AMD Ryzen 9 7950X, ESP32 @ 240MHz -->

## 💡 Proposed Optimization
<!-- Describe the low-level silicon/mathematical innovation: SIMD intrinsics, branchless logic, register optimization, LUT elimination, bit-twiddling hack, etc. -->

## 📈 Projected or Empirical Speedup
- **Projected Latency:** <!-- e.g., 0.28 ns/ray -->
- **Projected Speedup Factor:** <!-- e.g., 1.25x faster -->

## 🔍 Assembly / Code Diff
```diff
- // Current implementation
+ // Optimized hardware intrinsic implementation
```

## 🛡️ Invariant Check
- [ ] **Zero Dynamic Heap Allocations:** Does NOT call `malloc`, `free`, `new`, or `delete`.
- [ ] **Embedded Memory Budget:** Fits strictly within the 64 KB SRAM budget for microcontrollers.
- [ ] **Deterministic & Branchless:** Avoids non-deterministic branch speculation.
- [ ] **Mathematical Equivalence:** Passed all unit tests and safety checks in `./scripts/build_and_verify.sh`.

---

## 📬 Escalation & Direct Contact
> ⚠️ **Notice:**
> If you have an urgent silicon-level breakthrough, or in case you do not receive a response within **48–72 hours** on GitHub, please contact the author directly at:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Subject line: `[HALO-OPT] - <Summary of optimization>`)*
