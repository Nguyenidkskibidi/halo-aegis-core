---
name: "🚀 Yêu Cầu Tính Năng Mới (Tiếng Việt)"
about: Đề xuất tính năng mới, thuật toán robot, mô hình động lực học hoặc driver cảm biến.
title: "[TÍNH-NĂNG]: "
labels: ["enhancement"]
assignees: []
---

## 👤 Thông Tin Người Yêu Cầu
- **Tên GitHub (Username):** @ten-github-cua-ban <!-- Nhớ ghi username GitHub để tác giả dễ dàng tag, liên hệ và ghi nhận đóng góp -->

## 💡 Tóm Tắt Tính Năng Đề Xuất
<!-- Mô tả ngắn gọn, súc tích về tính năng hoặc năng lực mới mà bạn mong muốn có trong H.A.L.O. Aegis Core. -->

## 🤖 Ứng Dụng Thực Tế Trong Robot & Cứu Hộ Nhân Đạo
<!-- Tính năng này giải quyết vấn đề thực tế nào? Máy bay cứu nạn SAR, xe tự hành nhà kho AMR, robot 4 chân, tàu lặn không người lái AUV, xe tự lái, hay game mô phỏng AAA? -->

## 📐 Đề Xuất Thiết Kế Kiến Trúc
<!-- Tính năng này nên được tích hợp vào đâu trong H.A.L.O.? Tầng header nào sẽ bị ảnh hưởng? -->
- **Tầng Header:** <!-- Ví dụ: halo/sensors/, halo/kinodynamics/, halo/navigation/ -->
- **Ý tưởng thiết kế API:**
```cpp
// Chữ ký hàm hoặc cấu trúc API đề xuất
```

## 🛡️ Tuân Thủ Ràng Buộc Triệt Để (Zero-Overhead)
Vui lòng xác nhận đề xuất của bạn phù hợp với triết lý cốt lõi của dự án:
- [ ] **Zero Runtime Allocations:** Hoạt động hoàn toàn trên bộ đệm tĩnh hoặc pool nhớ được cấp phát trước, không cấp phát heap động.
- [ ] **Tương thích hệ nhúng vi điều khiển:** Khả thi trên chip giá rẻ ($2 như ESP32/STM32), không kéo theo thư viện cồng kềnh (không OpenCV, không PCL, không ROS core).
- [ ] **Ràng buộc đạo đức & nhân đạo:** Phục vụ hòa bình, cứu nạn, y tế, bảo tồn hoặc nghiên cứu khoa học dân sự (Giấy phép Hippocratic HL3).

---

## 📬 Kênh Liên Hệ Khẩn Cấp & Trực Tiếp
> ⚠️ **Lưu ý:**
> Trong trường hợp bạn **không nhận được phản hồi trong vòng 48–72 giờ** trên GitHub Issues, vui lòng liên hệ trực tiếp tác giả:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Tiêu đề email xin ghi rõ: `[HALO-FEAT] - Tóm tắt tính năng đề xuất`)*
