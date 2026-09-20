---
name: "🐛 Báo Cáo Lỗi (Tiếng Việt)"
about: Báo cáo hành vi bất thường, lỗi bộ nhớ, va chạm ngoài ý muốn hoặc lỗi tìm đường.
title: "[LỖI]: "
labels: ["bug", "triage"]
assignees: []
---

## 👤 Thông Tin Người Báo Cáo
- **Tên GitHub (Username):** @ten-github-cua-ban <!-- Nhớ ghi username GitHub để tác giả dễ dàng tag, liên hệ và ghi nhận đóng góp -->

## 📌 Mô Tả Lỗi
<!-- Mô tả rõ ràng, súc tích về sự cố bạn gặp phải. -->

## 💻 Môi Trường & Kiến Trúc Phần Cứng
- **Nền tảng phần cứng:** <!-- Ví dụ: Apple Silicon M3, Intel Core i9-13900K, Raspberry Pi 5, ESP32-WROOM-32, STM32H743 -->
- **Kiến trúc tập lệnh (ISA):** <!-- ARM64, x86_64, Xtensa LX6/LX7, RISC-V -->
- **Hệ điều hành / RTOS:** <!-- macOS 14+, Ubuntu 22.04/24.04, FreeRTOS, Bare-Metal không OS -->
- **Trình biên dịch & Phiên bản:** <!-- Clang++ 17+, AppleClang 15+, GCC 13+, ESP-IDF Clang -->
- **Cờ biên dịch (Build Flags):** <!-- Ví dụ: -O3 -march=native, -Os -DHALO_EMBEDDED_TARGET, -fsanitize=address,undefined -->

## 🔬 Đoạn Mã Tái Hiện Tối Giản (MRE)
<!-- Vui lòng cung cấp đoạn code C++20 ngắn gọn nhất, độc lập, có thể biên dịch chạy ngay để tái hiện lỗi. -->

```cpp
#include <halo/core/halo_supreme_core.h>
#include <iostream>

int main() {
  // Điền mã nguồn tái hiện tại đây
  return 0;
}
```

## 🔄 Các Bước Tái Hiện Sự Cố
1. Khởi tạo lưới không gian / cảm biến / động lực học với cấu hình ...
2. Gọi hàm `...` với tham số đầu vào `...`
3. Quan sát hiện tượng sập chương trình, crash, tràn bộ đệm, hoặc đường đi sai lệch.

## 🎯 Kết Quả Kỳ Vọng Đối Chiếu Kết Quả Thực Tế
- **Kết quả kỳ vọng:** <!-- Ví dụ: Tìm đường xong trong < 500 ns, 0% va chạm, quỹ đạo trơn tru C^3 -->
- **Kết quả thực tế:** <!-- Ví dụ: Lỗi Segmentation fault, ASan heap-buffer-overflow, vòng lặp vô tận, đâm vào vật cản -->

## 📊 Nhật Ký Kiểm Định / Sanitizer (Nếu Có)
<!-- Dán log đầu ra từ `./scripts/build_and_verify.sh`, AddressSanitizer (ASan), hoặc UndefinedBehaviorSanitizer (UBSan). -->
```text

```

---

## 📬 Kênh Liên Hệ Khẩn Cấp & Trực Tiếp
> ⚠️ **Lưu ý:**
> Trong trường hợp sự cố nghiêm trọng ảnh hưởng đến hệ thống robot đang hoạt động thực tế, hoặc **không nhận được phản hồi trong vòng 48–72 giờ** trên GitHub Issues, vui lòng nhắn tin trực tiếp tới tác giả:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Tiêu đề email xin ghi rõ: `[HALO-BUG] - Tóm tắt sự cố`)*
