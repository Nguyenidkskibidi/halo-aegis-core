---
name: "🚀 Feature Request (English)"
about: Suggest a new capability, robotics algorithm, kinodynamic model, or sensor driver.
title: "[FEAT]: "
labels: ["enhancement"]
assignees: []
---

## 👤 Submitter Info
- **GitHub Username:** @your-username <!-- Please provide your GitHub handle so we can tag, credit, and contact you easily -->

## 💡 Feature Summary
<!-- A concise explanation of the proposed feature or capability. -->

## 🤖 Real-World Robotics & SAR Use Case
<!-- What real-world robotics problem does this solve? Search & Rescue UAVs, AMRs, Legged Robots, Underwater AUVs, Autonomous Vehicles, AAA Game Engines? -->

## 📐 Proposed Architectural Design
<!-- How should this be integrated into H.A.L.O. Aegis Core? Which header files will be affected? -->
- **Header Layer:** <!-- e.g., halo/sensors/, halo/kinodynamics/, halo/navigation/ -->
- **Interface / API Concept:**
```cpp
// Proposed API signature
```

## 🛡️ Strict Zero-Overhead Compliance
Please confirm your proposal aligns with the core philosophy of H.A.L.O. Aegis Core:
- [ ] **Zero Runtime Allocations:** Must operate entirely on static buffers or caller-provided memory pools.
- [ ] **Embedded Compatible:** Must be feasible on low-power microcontrollers (ESP32/STM32) without requiring heavy dependencies (no OpenCV, no PCL, no ROS core libs).
- [ ] **Ethical Mandate:** Intended for peaceful, civilian, humanitarian, or scientific applications (HL3-CL-ECO-LAW-MIL-SUP-SV license).

---

## 📬 Escalation & Direct Contact
> ⚠️ **Notice:**
> In case you do not receive a response within **48–72 hours** on GitHub Issues, please reach out directly at:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Subject line: `[HALO-FEAT] - <Summary of feature>`)*
