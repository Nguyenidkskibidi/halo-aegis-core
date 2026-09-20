# 🛡️ Security Policy & Vulnerability Disclosure

> 🇻🇳 **Tài liệu tiếng Việt**: Đọc bản Tiếng Việt tại [SECURITY.vn.md](SECURITY.vn.md).

H.A.L.O. Aegis Core powers real-time autonomous systems, aerial drones (UAVs), and medical/search & rescue robotics. We take software safety, memory integrity, and ethical use extremely seriously.

---

## Supported Versions

| Version | Supported | Notes |
|---|---|---|
| `2.0.x` (Omni-Aegis) | ✅ Yes | Actively maintained; full 11-stage verification harness. |
| `< 2.0.0` | ❌ No | Deprecated; please upgrade to the unified Omni-Aegis engine. |

---

## 🔒 Reporting a Vulnerability or Memory Safety Hazard

If you discover a potential vulnerability, memory safety hazard, out-of-bounds access, or denial-of-service condition:

1. **Do NOT open a public GitHub issue** for undisclosed security vulnerabilities.
2. Please report the finding immediately to the project maintainer via direct email:
   - 📧 **Primary Security Contact:** `khoinguyennguyen683@gmail.com`
   - **Subject Line:** `[SECURITY-DISCLOSURE] - H.A.L.O. Aegis Core - <Short Description>`
3. **Information to Include:**
   - Detailed description of the vulnerability and attack/hazard vector.
   - Minimal C++20 reproducible code snippet or input payload.
   - Target architecture (x86_64, ARM64, ESP32, STM32) and compiler flags.
   - AddressSanitizer (ASan) or UndefinedBehaviorSanitizer (UBSan) logs if available.
4. **Response Timeline:**
   - Acknowledgement within **24–48 hours**.
   - Remediation patch and verification within **7 business days**.
   - Public credit and CVE coordination (if applicable) upon release of the fix.

---

## 📜 Ethical Misuse Reporting

H.A.L.O. Aegis Core is licensed under the **Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV**. Use for weaponized platforms, offensive warfare, autonomous kinetic targeting, or mass surveillance is strictly prohibited. If you observe unauthorized usage violating this ethical mandate, please notify: `khoinguyennguyen683@gmail.com`.
