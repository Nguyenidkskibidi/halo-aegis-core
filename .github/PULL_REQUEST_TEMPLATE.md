<!-- 🇻🇳 Bản Tiếng Việt có sẵn tại: .github/PULL_REQUEST_TEMPLATE.vn.md -->

## 👤 Author Info
- **GitHub Username:** @your-username

## 📝 Summary of Changes
<!-- Brief 2-3 sentence overview of what this Pull Request introduces, fixes, or optimizes. -->

## 🎯 Motivation & Context
<!-- Why is this change necessary? Reference linked issues (e.g., Fixes #123). -->

## 🛡️ Mechanical Sympathy & Zero-Overhead Invariants
Please verify that your changes adhere to H.A.L.O. Aegis Core core principles:
- [ ] **Zero Runtime Allocation:** 0 calls to `malloc`, `free`, `new`, `delete` during execution.
- [ ] **Embedded Footprint:** Release binary size strictly under **40 KB** (`-Os -flto`).
- [ ] **Static SRAM Envelope:** Embedded execution stays within **64 KB** SRAM budget.
- [ ] **C++20 Header-Only:** Clean headers without external dependencies (OpenCV, PCL, ROS).
- [ ] **Ethical Mandate:** Compliant with Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV.

## 🧪 Verification & Acceptance Pipeline
Please run the master verification pipeline locally and confirm all checks pass:
```bash
./scripts/build_and_verify.sh
```
- [ ] `[1/11]` Clang-Format Invariant (100% compliant)
- [ ] `[2/11]` Clang-Tidy Static Analysis (0 safety risks, 0 logic bugs)
- [ ] `[3/11]` Assembly Generation Audit (Zero heap spills in hot intrinsics)
- [ ] `[4/11]` ASan & UBSan Memory Safety (0 leaks, 0 UB)
- [ ] `[5/11]` Flash Binary Footprint (< 40,960 bytes)
- [ ] `[6/11]` Embedded Dynamic Flight Simulation (0.00% collisions)
- [ ] `[7/11]` Sub-Microsecond Hardware Maximization Suite (< 0.35 ns raycast)
- [ ] `[8/11]` Universal Spatial Benchmark (<= 16.00 MB RAM cap)
- [ ] `[9/11]` Embedded & ESP32 Zero-Heap Static Test (< 64 KB SRAM)
- [ ] `[10/11]` Omni-Aegis Universal Genius Benchmark (All 4 physical gates passed)
- [ ] `[11/11]` Google Benchmark Suite (All microbenchmarks green)

---

## 📬 Escalation & Direct Contact
> ⚠️ **Notice:**
> If your Pull Request requires urgent review or you do not receive a response within **48–72 hours**, please reach out directly at:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Subject line: `[HALO-PR] - <PR title / number>`)*
