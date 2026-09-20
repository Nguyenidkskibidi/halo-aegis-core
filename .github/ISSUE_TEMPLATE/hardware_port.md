---
name: "📟 Hardware / MCU Port (English)"
about: Propose or report validation of H.A.L.O. Aegis Core on a new microcontroller or SBC.
title: "[PORT]: "
labels: ["embedded", "hardware-port"]
assignees: []
---

## 👤 Submitter Info
- **GitHub Username:** @your-username <!-- Please provide your GitHub handle so we can tag, credit, and contact you easily -->

## 🔬 Hardware Target Details
- **Target Chip / SoC:** <!-- e.g., ESP32-S3, STM32H753ZI, RP2040, Teensy 4.1, Kendryte K210, Jetson Orin Nano -->
- **Core Architecture:** <!-- e.g., ARM Cortex-M7, Xtensa LX7, RISC-V 32-bit (RV32IMAFDC), ARM Cortex-A78 -->
- **Clock Frequency:** <!-- e.g., 240 MHz, 480 MHz, 133 MHz -->
- **Available SRAM / Flash:** <!-- e.g., 512 KB SRAM / 4 MB Flash -->
- **Hardware FPU:** <!-- Single-precision FPU, Double-precision FPU, or Soft-FPU (Fixed32 only) -->

## 🛠️ Build Environment & Toolchain
- **Framework / SDK:** <!-- ESP-IDF v5.x, STM32CubeIDE, PlatformIO, Arduino-ESP32, Zephyr RTOS -->
- **Compiler Version:** <!-- e.g., riscv32-esp-elf-gcc 13.2, arm-none-eabi-gcc 12.3, Clang 18 -->
- **Optimization Flags:** <!-- e.g., -Os, -O3, -flto, -ffunction-sections, -fdata-sections -->

## 📊 Footprint & Validation Results
- **Flash ROM Consumed:** <!-- e.g., 34.3 KB (< 40 KB target) -->
- **Static SRAM Consumed:** <!-- e.g., 57.4 KB (< 64 KB budget) -->
- **Routing Latency on MCU:** <!-- e.g., 185 µs on 240MHz core -->
- **Zero-Heap Invariant Verified:** <!-- Did you confirm 0 heap malloc calls during routing? YES / NO -->

## 📝 Observations & Architecture-Specific Notes
<!-- Any architecture-specific quirks (e.g., unaligned memory access penalties, endianness, interrupt latency, timer precision)? -->

---

## 📬 Escalation & Direct Contact
> ⚠️ **Notice:**
> In case you do not receive a response within **48–72 hours** on GitHub Issues, please reach out directly at:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Subject line: `[HALO-PORT] - <Target MCU / Hardware>`)*
