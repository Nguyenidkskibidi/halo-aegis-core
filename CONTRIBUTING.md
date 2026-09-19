# 🤝 Contributing to H.A.L.O. Aegis Core

> 🌐 **Language / Ngôn ngữ**: **English** | [Tiếng Việt](CONTRIBUTING.vn.md)

Hi there! 👋 Welcome to the **H.A.L.O. Aegis Core** project.

First of all, thank you from the bottom of my heart for checking out this repository! My name is **Nguyễn Khôi Nguyên**, and I am a secondary school student with a deep passion for low-level C++, computational geometry, and physics. I built this core with a single humanitarian mission: **to help the Red Cross, emergency responders, and search-and-rescue (SAR) teams save lives through ultra-fast, lightweight autonomous robotics and drone technology.**

Since I am still a student and a continuous learner, I know my code might not be flawless. I truly value your expertise, mentorship, and technical feedback to make this tool even more reliable, fast, and bulletproof.

---

## 🛡️ Code of Conduct

I believe in a community that is collaborative, welcoming, and grounded in mutual respect:

- **Be Kind & Respectful**: Please use constructive, professional, and encouraging language.
- **Focus on the Engineering**: If you find a bug, race condition, or a flaw in mathematical logic, I am all ears! I am eager to learn from your experience.
- **Honor the Humanitarian Mission**: This project exists to empower rescue operations and low-cost embedded hardware in developing regions where resources are constrained. Every byte of RAM and every CPU cycle saved matters.
- **Ethical License Adherence**: All contributions must strictly comply with the **Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV**. We strictly forbid offensive weapons, autonomous lethal platforms, or human surveillance tooling.

---

## ⚡ The 5 Sacred Bare-Metal Engineering Invariants

Before writing a single line of code, please ensure your contribution honors the sacred architectural rules of `H.A.L.O. Aegis Core`:

### 1. Zero Runtime Heap Allocations
- **NEVER** invoke `malloc`, `free`, `new`, or `delete` in any runtime navigation or collision avoidance routine.
- **NEVER** use heap-backed containers like `std::vector`, `std::map`, `std::string`, or `std::list` inside the engine core.
- Use the pre-faulted `halo::memory::ArenaAllocator` or contiguous stack/NTTP arrays.

### 2. Strict 64-Byte Cache Line Alignment
- Align all critical data structures to 64 bytes (`alignas(64)`) to fit exactly within hardware CPU cache lines (L1D).
- Eliminate cache-line boundary splits and false sharing.

### 3. Absolute Eradication of `<iostream>` Bloat
- **DO NOT** `#include <iostream>`, `std::cout`, `std::endl`, or `std::format` in core headers.
- Standard C++ stream formatting introduces hundreds of kilobytes of runtime vtables and metadata bloat.
- Use the zero-overhead `HALO_LOG` macro (which strips away entirely to `((void)0)` in Release builds) or minimal C-style formatting (`std::printf`) in CLI tools.

### 4. Zero-Tolerance Anti-Fabrication & Anti-DCE Memory Barriers
- Never mock benchmark numbers, fake test outputs, or introduce empty loops.
- Benchmarks must use the hardware memory sink `DoNotOptimize(val)`:
  ```cpp
  template <typename T>
  [[gnu::always_inline]] inline void DoNotOptimize(T const& val) {
    asm volatile("" : : "g"(val) : "memory");
  }
  ```
- Every reported metric must be directly measured via high-resolution monotonic hardware counters.

### 5. Extreme Binary Footprint (< 40 KB Release Target)
- The stripped embedded Release binary must strictly stay under **40,960 bytes (40 KB)**.
- Compile with `-fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto` to allow dead-stripping of unused symbols.

---

## 🛠️ How You Can Help

You are invited to contribute in many meaningful ways:

1. **Reporting Bugs & Edge Cases**:
   - Encountered a crash, boundary wrap, or infinite loop in a corner case? Open an issue with a minimal reproduction test case!
2. **Microarchitecture & SIMD Optimizations**:
   - Have ideas to optimize the $0.34\text{ ns}$ raycast even further on ARM NEON, AVX-512, or RISC-V Vector Extensions? We would love to review your assembly!
3. **Cross-Platform Embedded Testing**:
   - Help test and validate H.A.L.O. on physical boards: STM32H7, ESP32-S3, Raspberry Pi CM4, NVIDIA Jetson Orin Nano, or RISC-V SBCs.
4. **Any-Angle Pathfinding & Kinematics**:
   - Improvements to SSFA string pulling, spline curvature constraints, or wind-field drift compensation.
5. **Code Cleanup & Refactoring**:
   - Found any confusing naming or messy logic? Clean and concise refactorings are warmly welcomed.

---

## 📬 Pull Request (PR) Workflow

1. **Open an Issue First**:
   - Discuss your proposed changes or feature ideas before writing large chunks of code.
2. **Create a Feature Branch**:
   ```bash
   git checkout -b feature/my-awesome-optimization
   ```
3. **Verify Locally Against the 5-Stage Gate**:
   - Before submitting, run the full automated verification pipeline:
     ```bash
     ./scripts/build_and_verify.sh
     ```
   - **Must pass 100%**:
     - Stage 1: ASan & UBSan clean (0 memory leaks, 0 undefined behaviors).
     - Stage 2: Stripped binary size $< 40\text{ KB}$.
     - Stage 3: Dynamic drone flight simulation (0.00% collisions across 5,000 cycles).
     - Stage 4: Hardware maximization suite ($< 0.35\text{ ns}$ raycast, P99 $< 500\text{ ns}$ JPS+).
     - Stage 5: Universal spatial benchmark (total RAM $\le 16.00\text{ MB}$).
4. **Submit Your PR**:
   - Provide a clear summary of what you changed, why you changed it, and include benchmark comparisons.
5. **Wait for Review**:
   - I will review your PR as soon as I finish my homework, exams, or current coding sprint! 🥤

---

## ❤️ Final Word

I am deeply grateful for your time, intellect, and passion. Together, let's build the fastest, most reliable open-source robotics core in the world—saving lives one nanosecond at a time.

> *"Stay hungry, stay humble, and keep optimizing."* 🚀✨