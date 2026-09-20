---
name: "🐛 Bug Report (English)"
about: Report an unexpected behavior, memory issue, collision, or pathing failure.
title: "[BUG]: "
labels: ["bug", "triage"]
assignees: []
---

## 👤 Submitter Info
- **GitHub Username:** @your-username <!-- Please provide your GitHub handle so we can tag, credit, and contact you easily -->

## 📌 Bug Description
<!-- A clear and concise description of what the bug is. -->

## 💻 Target Architecture & Environment
- **Hardware Platform:** <!-- e.g., Apple Silicon M3, Intel Core i9-13900K, Raspberry Pi 5, ESP32-WROOM-32, STM32H743 -->
- **Instruction Architecture:** <!-- ARM64, x86_64, Xtensa LX6/LX7, RISC-V -->
- **Operating System / RTOS:** <!-- macOS 14+, Ubuntu 22.04/24.04, FreeRTOS, Bare-Metal -->
- **Compiler & Version:** <!-- Clang++ 17+, AppleClang 15+, GCC 13+, ESP-IDF Clang -->
- **Build Flags:** <!-- e.g., -O3 -march=native, -Os -DHALO_EMBEDDED_TARGET, -fsanitize=address,undefined -->

## 🔬 Minimal Reproducible Example (MRE)
<!-- Please provide a minimal self-contained C++20 snippet that triggers the bug. -->

```cpp
#include <halo/core/halo_supreme_core.h>
#include <iostream>

int main() {
  // Reproduce code here
  return 0;
}
```

## 🔄 Steps to Reproduce
1. Configure grid/sensor/kinodynamics with ...
2. Call method `...` with inputs `...`
3. Observe crash, assertion, out-of-bounds access, or sub-optimal waypoint.

## 🎯 Expected vs. Actual Result
- **Expected Result:** <!-- e.g., Route found in < 500 ns, 0 collisions, smooth C^3 trajectory -->
- **Actual Result:** <!-- e.g., Segmentation fault, ASan heap-buffer-overflow, infinite loop, collision detected -->

## 📊 Pipeline / Sanitizer Telemetry (If Applicable)
<!-- Paste relevant output from `./scripts/build_and_verify.sh`, AddressSanitizer (ASan), or UndefinedBehaviorSanitizer (UBSan) logs. -->
```text

```

---

## 📬 Escalation & Direct Contact
> ⚠️ **Notice:**
> If your issue is critical for active robotics deployments, or in case you do not receive a response within **48–72 hours** on GitHub Issues, please contact the project author directly at:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Subject line: `[HALO-BUG] - <Summary of issue>`)*
