<!-- 🇬🇧 English version available at: .github/PULL_REQUEST_TEMPLATE.md -->

## 👤 Thông Tin Tác Giả PR
- **Tên GitHub (Username):** @ten-github-cua-ban <!-- Nhớ ghi username GitHub để tác giả dễ dàng tag và trao đổi trực tiếp -->

## 📝 Tóm Tắt Các Thay Đổi
<!-- Tóm tắt ngắn gọn trong 2-3 câu về những gì Pull Request này bổ sung, sửa lỗi hoặc tối ưu hóa. -->

## 🎯 Bối Cảnh & Động Lực
<!-- Vì sao thay đổi này lại cần thiết? Dẫn link issue liên quan nếu có (Ví dụ: Đóng #123). -->

## 🛡️ Ràng Buộc Cơ Học & Chuẩn Zero-Overhead
Vui lòng kiểm tra và xác nhận mã nguồn của bạn tuân thủ các nguyên lý cốt lõi của H.A.L.O. Aegis Core:
- [ ] **Zero Runtime Allocation:** Tuyệt đối không gọi `malloc`, `free`, `new`, `delete` khi đang chạy.
- [ ] **Kích thước file nhị phân:** Bản build release stripped phải dưới **40 KB** (`-Os -flto`).
- [ ] **Ngân sách SRAM tĩnh:** Chế độ nhúng hoạt động trọn vẹn trong trần **64 KB** SRAM.
- [ ] **Header-Only C++20:** Header độc lập, sạch sẽ, không kéo theo thư viện ngoài cồng kềnh (không OpenCV, không PCL, không ROS).
- [ ] **Ràng buộc đạo đức:** Tuân thủ giấy phép nhân đạo Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV.

## 🧪 Quy Trình Build & Kiểm Định Chấp Thuận
Vui lòng chạy bộ kiểm định thống nhất 11 tầng tại máy cục bộ và đánh dấu toàn bộ các mục đã vượt qua:
```bash
./scripts/build_and_verify.sh
```
- [ ] `[1/11]` Chuẩn Format Clang-Format (100% tuân thủ)
- [ ] `[2/11]` Phân tích tĩnh Clang-Tidy (0 rủi ro an toàn, 0 lỗi logic)
- [ ] `[3/11]` Kiểm toán mã máy Assembly (Zero heap spills trong các hàm nội tại)
- [ ] `[4/11]` An toàn bộ nhớ ASan & UBSan (0 rò rỉ, 0 undefined behavior)
- [ ] `[5/11]` Kích thước Flash nhúng (< 40,960 bytes)
- [ ] `[6/11]` Mô phỏng bay nhúng thời gian thực (0.00% va chạm qua 5.000 chu kỳ)
- [ ] `[7/11]` Cổng đo tối đa hóa phần cứng (< 0.35 ns/tia)
- [ ] `[8/11]` Kiểm chuẩn không gian đại lục & đô thị 3D (<= 16.00 MB RAM)
- [ ] `[9/11]` Kiểm chuẩn vi điều khiển & ESP32 (< 64 KB SRAM)
- [ ] `[10/11]` 4 Cổng kiểm chuẩn vật lý Project Omni-Aegis (Toàn bộ PASSED)
- [ ] `[11/11]` Bộ đo chuẩn công nghiệp Google Benchmark (13/13 benchmark xanh)

---

## 📬 Kênh Phản Hồi Trực Tiếp & Khẩn Cấp
> ⚠️ **Lưu ý:**
> Trong trường hợp Pull Request của bạn cần review gấp hoặc **không nhận được phản hồi trong vòng 48–72 giờ**, vui lòng nhắn trực tiếp cho tác giả:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Tiêu đề email xin ghi rõ: `[HALO-PR] - <Tiêu đề / Số PR>`)*
