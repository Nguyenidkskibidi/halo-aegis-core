---
name: "📟 Báo Cáo Port Phần Cứng & Vi Điều Khiển (Tiếng Việt)"
about: Báo cáo kết quả thử nghiệm hoặc đề xuất port H.A.L.O. lên vi điều khiển / SBC mới.
title: "[PORT]: "
labels: ["embedded", "hardware-port"]
assignees: []
---

## 👤 Thông Tin Người Đóng Góp
- **Tên GitHub (Username):** @ten-github-cua-ban <!-- Nhớ ghi username GitHub để tác giả dễ dàng tag, liên hệ và ghi nhận đóng góp -->

## 🔬 Thông Số Phần Cứng Mục Tiêu
- **Dòng chip / SoC:** <!-- Ví dụ: ESP32-S3, STM32H753ZI, RP2040, Teensy 4.1, Kendryte K210, Jetson Orin Nano -->
- **Kiến trúc lõi:** <!-- Ví dụ: ARM Cortex-M7, Xtensa LX7, RISC-V 32-bit (RV32IMAFDC), ARM Cortex-A78 -->
- **Xung nhịp vi xử lý:** <!-- Ví dụ: 240 MHz, 480 MHz, 133 MHz -->
- **Dung lượng SRAM / Flash:** <!-- Ví dụ: 512 KB SRAM / 4 MB Flash -->
- **Bộ tính toán số thực FPU:** <!-- FPU đơn chính xác, FPU kép chính xác, hoặc Soft-FPU (dùng Fixed32 hoàn toàn) -->

## 🛠️ Môi Trường Biên Dịch & Công Cụ (Toolchain)
- **Framework / SDK:** <!-- ESP-IDF v5.x, STM32CubeIDE, PlatformIO, Arduino-ESP32, Zephyr RTOS -->
- **Trình biên dịch & Phiên bản:** <!-- Ví dụ: riscv32-esp-elf-gcc 13.2, arm-none-eabi-gcc 12.3, Clang 18 -->
- **Cờ tối ưu hóa:** <!-- Ví dụ: -Os, -O3, -flto, -ffunction-sections, -fdata-sections -->

## 📊 Kết Quả Đo Đạc Bộ Nhớ & Tốc Độ Thực Tế
- **Dung lượng Flash ROM tiêu thụ:** <!-- Ví dụ: 34.3 KB (đạt chuẩn < 40 KB) -->
- **Dung lượng SRAM tĩnh tiêu thụ:** <!-- Ví dụ: 57.4 KB (đạt chuẩn < 64 KB) -->
- **Độ trễ tính đường trên vi điều khiển:** <!-- Ví dụ: 185 µs trên lõi 240MHz -->
- **Kiểm định Zero-Heap:** <!-- Đã xác nhận 0 lần gọi malloc khi đang chạy chưa? CÓ / KHÔNG -->

## 📝 Quan Sát & Lưu Ý Kỹ Thuật Vi Kiến Trúc
<!-- Các đặc tính riêng của phần cứng (Ví dụ: bẫy bộ nhớ không căn chỉnh alignment fault, thứ tự byte endianness, độ trễ ngắt timer)? -->

---

## 📬 Kênh Liên Hệ Khẩn Cấp & Trực Tiếp
> ⚠️ **Lưu ý:**
> Trong trường hợp bạn **không nhận được phản hồi trong vòng 48–72 giờ** trên GitHub Issues, vui lòng liên hệ trực tiếp tác giả:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Tiêu đề email xin ghi rõ: `[HALO-PORT] - Tên vi điều khiển / phần cứng`)*
