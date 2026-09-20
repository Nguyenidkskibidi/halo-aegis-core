# 🛡️ Chính Sách Bảo Mật & Tiết Lộ Lỗ Hổng Kỹ Thuật

> 🇬🇧 **English Version**: Read the English version at [SECURITY.md](SECURITY.md).

H.A.L.O. Aegis Core là lõi điều hướng phục vụ các hệ thống tự hành thời gian thực, máy bay cứu hộ cứu nạn không người lái (UAV SAR) và robot y tế. Chúng tôi đặt tính an toàn phần mềm, toàn vẹn bộ nhớ và tính chuẩn mực đạo đức lên hàng đầu.

---

## Các Phiên Bản Được Hỗ Trợ

| Phiên Bản | Được Hỗ Trợ | Ghi Chú |
|---|---|---|
| `2.0.x` (Omni-Aegis) | ✅ Có | Được bảo trì tích cực; vượt qua toàn bộ 11 tầng kiểm định. |
| `< 2.0.0` | ❌ Không | Ngừng hỗ trợ; vui lòng nâng cấp lên bản Omni-Aegis thống nhất. |

---

## 🔒 Báo Cáo Lỗ Hổng Bảo Mật & Nguy Cơ Mất An Toàn Bộ Nhớ

Nếu bạn phát hiện lỗ hổng tiềm ẩn, lỗi an toàn bộ nhớ (tràn bộ đệm, use-after-free), hoặc nguy cơ treo đơ hệ thống (denial-of-service):

1. **Tuyệt đối KHÔNG mở Issue công khai trên GitHub** cho các lỗ hổng chưa được vá.
2. Vui lòng gửi thông tin bảo mật trực tiếp tới tác giả dự án qua email:
   - 📧 **Kênh Bảo Mật Chính:** `khoinguyennguyen683@gmail.com`
   - **Tiêu Đề Email:** `[SECURITY-DISCLOSURE] - H.A.L.O. Aegis Core - <Tóm tắt nguy cơ>`
3. **Thông Tin Cần Cung Cấp:**
   - Mô tả chi tiết lỗ hổng và kịch bản có thể dẫn tới sự cố.
   - Đoạn mã C++20 tối giản hoặc dữ liệu đầu vào kích hoạt sự cố.
   - Nền tảng kiến trúc phần cứng (x86_64, ARM64, ESP32, STM32) và cờ biên dịch.
   - Nhật ký lỗi từ AddressSanitizer (ASan) hoặc UndefinedBehaviorSanitizer (UBSan) nếu có.
4. **Quy Trình & Thời Gian Phản Hồi:**
   - Xác nhận đã nhận thông tin trong vòng **24–48 giờ**.
   - Phát hành bản vá khắc phục và kiểm định trong vòng **7 ngày làm việc**.
   - Vinh danh người đóng góp và phối hợp đăng ký mã CVE (nếu cần) khi bản vá được phát hành chính thức.

---

## 📜 Kênh Tố Cáo Vi Phạm Giấy Phép Nhân Đạo

H.A.L.O. Aegis Core được bảo hộ dưới giấy phép **Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV**. Việc sử dụng mã nguồn này cho vũ khí sát thương, chiến tranh xâm lược, hệ thống nhắm mục tiêu tự động hoặc giám sát vi phạm nhân quyền đều bị nghiêm cấm tuyệt đối. Nếu phát hiện hành vi lạm dụng vi phạm giấy phép, vui lòng thông báo về: `khoinguyennguyen683@gmail.com`.
