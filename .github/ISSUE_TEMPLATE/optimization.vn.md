---
name: "⚡ Đề Xuất Tối Ưu Hiệu Năng (Tiếng Việt)"
about: Đề xuất cải tiến cấp độ vi kiến trúc silicon, mẹo SIMD, hoặc tối ưu cache.
title: "[TỐI-ƯU]: "
labels: ["performance", "optimization"]
assignees: []
---

## 👤 Thông Tin Người Đề Xuất
- **Tên GitHub (Username):** @ten-github-cua-ban <!-- Nhớ ghi username GitHub để tác giả dễ dàng tag, liên hệ và ghi nhận đóng góp -->

## 🎯 Phân Hệ & Hàm Mục Tiêu
<!-- Bạn muốn tối ưu hàm hoặc phân hệ nào? -->
- [ ] Bắn tia Bitboard SWAR / SIMD (`halo_simd.h`, `halo_swar_10_layer_bitboard.h`)
- [ ] Định tuyến siêu tốc JPS+ (`halo_jps_plus.h`)
- [ ] Giải quỹ đạo đa thức bậc 5 Kinodynamic (`halo_kinodynamics.h`)
- [ ] Nạp cảm biến đa hình không copy (`halo_sensor_fusion.h`)
- [ ] Động cơ tính toán dấu phẩy tĩnh Fixed32 / Sqrt (`halo_fixed_point.h`)
- [ ] Bố cục bộ nhớ / Căn chỉnh Cache Line / Chuẩn Zero-Heap (`halo_memory.h`)
- [ ] Khác: <!-- Nêu rõ tại đây -->

## ⏱️ Thông Số Đo Đạc Chuẩn Hiện Tại (Baseline)
<!-- Dán số liệu từ `./scripts/build_and_verify.sh` hoặc Google Benchmark `tests/halo_google_benchmark.cpp` -->
- **Hàm / Bài benchmark mục tiêu:** <!-- Ví dụ: BM_SWAR_RaycastRow hoặc BM_Kinodynamics_PurePursuit -->
- **Độ trễ hiện tại:** <!-- Ví dụ: 0.35 ns/tia, hoặc 25.5 ns/bước -->
- **Thông lượng hiện tại:** <!-- Ví dụ: 2.31 tỷ phép tính/giây (2.31 G ops/s) -->
- **Phần cứng thử nghiệm:** <!-- Ví dụ: Apple Silicon M3 Pro, AMD Ryzen 9 7950X, ESP32 @ 240MHz -->

## 💡 Đề Xuất Đột Phá Kỹ Thuật
<!-- Mô tả giải pháp phần cứng / toán học: Lệnh nội tại SIMD, logic không phân nhánh (branchless), tối ưu thanh ghi vi xử lý, khử bảng LUT, thủ thuật bit-twiddling, v.v. -->

## 📈 Mức Độ Tăng Tốc Dự Kiến Hoặc Thực Tế
- **Độ trễ sau tối ưu:** <!-- Ví dụ: 0.28 ns/tia -->
- **Hệ số tăng tốc (Speedup Factor):** <!-- Ví dụ: 1.25x (nhanh hơn 25%) -->

## 🔍 So Sánh Mã Máy Assembly Hoặc Mã C++ (Diff)
```diff
- // Mã nguồn hiện tại
+ // Mã nguồn tối ưu bằng lệnh phần cứng nội tại
```

## 🛡️ Kiểm Tra Các Ràng Buộc Bất Biến
- [ ] **Zero Dynamic Heap Allocations:** Tuyệt đối KHÔNG gọi `malloc`, `free`, `new`, hoặc `delete`.
- [ ] **Ngân sách nhúng:** Nằm trọn vẹn trong trần 64 KB SRAM của vi điều khiển.
- [ ] **Xác định & Không rẽ nhánh:** Tránh dự đoán nhánh sai của CPU (branch misprediction).
- [ ] **Tính tương đương toán học:** Vượt qua toàn bộ 11 tầng kiểm định trong `./scripts/build_and_verify.sh`.

---

## 📬 Kênh Liên Hệ Khẩn Cấp & Trực Tiếp
> ⚠️ **Lưu ý:**
> Nếu bạn có một giải pháp tối ưu đột phá mang tính cách mạng cho phần cứng, hoặc **không nhận được phản hồi trong vòng 48–72 giờ** trên GitHub, vui lòng liên hệ trực tiếp tác giả:
> 
> 📧 **Email:** `khoinguyennguyen683@gmail.com`  
> *(Tiêu đề email xin ghi rõ: `[HALO-OPT] - Tóm tắt đề xuất tối ưu`)*
