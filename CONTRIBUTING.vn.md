# 🤝 Hướng Dẫn Đóng Góp Vào H.A.L.O. Aegis Core

> 🌐 **Ngôn ngữ**: [English](CONTRIBUTING.md) | **Tiếng Việt**

Chào bạn! 👋 Chào mừng bạn đến với dự án **H.A.L.O. Aegis Core**.

Lời đầu tiên, mình xin gửi lời cảm ơn chân thành và sâu sắc nhất vì bạn đã ghé thăm kho mã nguồn này! Mình tên là **Nguyễn Khôi Nguyên**, hiện là một học sinh cấp 2 mang niềm đam mê cháy bỏng với C++ cấp thấp (low-level systems), hình học tính toán và vật lý. Mình xây dựng lõi điều hướng này với một sứ mệnh nhân đạo duy nhất: **giúp đỡ Hội Chữ Thập Đỏ, các đội cứu hộ khẩn cấp và lực lượng tìm kiếm cứu nạn (SAR) cứu sống được nhiều sinh mạng hơn thông qua công nghệ robot và drone tự hành siêu tốc, siêu nhẹ.**

Vì mình vẫn còn là một học sinh và đang trên con đường không ngừng học hỏi, mình biết mã nguồn của mình chắc chắn chưa thể hoàn hảo. Mình vô cùng trân trọng kinh nghiệm, sự cố vấn và mọi góp ý kỹ thuật từ bạn để giúp cỗ máy này ngày càng tin cậy, chuẩn xác và bất khả chiến bại hơn.

---

## 🛡️ Quy Tắc Ứng Xử (Code of Conduct)

Mình tin tưởng vào một cộng đồng kỹ sư cởi mở, hỗ trợ lẫn nhau và xây dựng trên tinh thần tôn trọng:

- **Lịch sự & Tôn trọng**: Vui lòng sử dụng ngôn từ hòa nhã, mang tính xây dựng và tích cực.
- **Tập trung vào Chuyên môn Kỹ thuật**: Nếu bạn phát hiện một lỗi tràn bộ nhớ, race condition hay sai sót trong logic toán học, mình luôn lắng nghe! Mình rất mong được học hỏi từ kinh nghiệm thực chiến của các bậc tiền bối.
- **Tôn trọng Sứ mệnh Nhân đạo**: Dự án này hướng đến việc hỗ trợ người dân và các đội cứu hộ tại các quốc gia đang phát triển nơi phần cứng máy bay và máy tính nhúng còn rất hạn chế. Mỗi byte RAM hay mỗi chu kỳ CPU tiết kiệm được đều có thể đổi lấy một mạng sống.
- **Tuân thủ Giấy phép Đạo đức**: Mọi đóng góp phải nghiêm túc tuân thủ **Giấy phép Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV**. Nghiêm cấm tuyệt đối việc sử dụng hoặc đóng góp mã nguồn phục vụ mục đích vũ khí quân sự sát thương, tấn công chiến tranh hay công cụ giám sát xâm phạm quyền con người.

---

## ⚡ 5 Bất Biến Kỹ Thuật Bare-Metal Bất Khả Xâm Phạm

Trước khi viết bất kỳ dòng mã nào, vui lòng đảm bảo rằng giải pháp của bạn tuân thủ nghiêm ngặt 5 nguyên tắc kiến trúc cốt lõi của `H.A.L.O.`:

### 1. Tuyệt Đối Không Cấp Phát Bộ Nhớ Động Tại Runtime (Zero Heap Allocations)
- **CẤM** gọi `malloc`, `free`, `new`, hoặc `delete` trong bất kỳ luồng xử lý điều hướng hay né va chạm thời gian thực nào.
- **CẤM** sử dụng các cấu trúc dữ liệu tự co giãn trên Heap của STL như `std::vector`, `std::map`, `std::string`, `std::list` trong lõi động cơ.
- Mọi bộ nhớ phải được lấy từ vùng nhớ đơn điệu đã làm ấm sẵn `halo::memory::ArenaAllocator` hoặc mảng cố định trên Stack/NTTP.

### 2. Căn Chỉnh Cache Line 64-Byte Tuyệt Đối
- Mọi cấu trúc dữ liệu trọng yếu đều phải được căn chỉnh 64 byte (`alignas(64)`) để vừa khít vào 1 dòng bộ nhớ đệm L1D của CPU.
- Triệt tiêu hoàn toàn hiện tượng phân mảnh đường biên cache (cache split) và xung đột luồng giả (false sharing).

### 3. Xóa Bỏ Hoàn Toàn Rác Thư Viện `<iostream>`
- **KHÔNG ĐƯỢC** `#include <iostream>`, gọi `std::cout`, `std::endl`, hay `std::format` trong mã nguồn lõi.
- Cơ chế stream của C++ kéo theo hàng trăm KB metadata, bảng vtable ảo và mã định dạng cồng kềnh.
- Hãy sử dụng macro không chi phí `HALO_LOG` (tự động biến mất thành `((void)0)` trong bản Release) hoặc hàm C thuần (`std::printf`) trong các công cụ dòng lệnh demo.

### 4. Chống Tự Huyễn Của Compiler & Rào Cản Bộ Nhớ ASM (Anti-DCE)
- Tuyệt đối không làm giả số liệu benchmark, không mock kết quả, không viết vòng lặp rỗng.
- Mọi phép đo hiệu năng phần cứng đều phải được bảo vệ bằng rào cản Assembly:
  ```cpp
  template <typename T>
  [[gnu::always_inline]] inline void DoNotOptimize(T const& val) {
    asm volatile("" : : "g"(val) : "memory");
  }
  ```
- Mọi số liệu đo lường phải xuất phát từ bộ đếm phần cứng đơn điệu thực tế (`clock_gettime_nsec_np` / `CLOCK_MONOTONIC_RAW`).

### 5. Trần Kích Thước File Nhị Phân Siêu Nhỏ (< 40 KB Release)
- File thực thi nhúng sau khi `strip` symbol bắt buộc phải nằm dưới ngưỡng trần cứng **40,960 bytes (40 KB)**.
- Biên dịch ở cờ `-fno-rtti -fno-exceptions -ffunction-sections -fdata-sections -flto` để loại bỏ các hàm và symbol không dùng đến.

### 6. Khả Năng Tương Thích Tuyệt Đối Trên Vi Điều Khiển Nhúng (Zero OS Pollution)
- **CẤM** include trực tiếp các header của hệ điều hành máy tính (`<sys/mman.h>`, `<pthread.h>`, `<sched.h>`) mà không bọc trong cờ kiểm tra nền tảng (`#if !defined(ESP_PLATFORM) && !defined(ARDUINO)...`).
- Mã nguồn phải biên dịch mượt mà trên kiến trúc 32-bit (ESP32 Xtensa LX6/LX7, RISC-V 32, ARM Cortex-M) ở cả chuẩn C++17 và C++20.
- Mọi thuật toán phải hỗ trợ cơ chế vùng đệm tĩnh xác thực (`BootSystemWithBuffer`) để vi điều khiển không bao giờ phải dựa vào phân trang bộ nhớ ảo của hệ điều hành.

---

## 🛠️ Bạn Có Thể Đóng Góp Như Thế Nào?

Bạn có thể tham gia đóng góp bằng rất nhiều cách ý nghĩa:

1. **Báo Lỗi & Tình Huống Góc Cạnh (Edge Cases)**:
   - Phát hiện hiện tượng văng ứng dụng, lỗi tràn biên tọa độ, hay vòng lặp vô tận? Hãy mở Issue kèm mã tái hiện tối giản!
2. **Tối Ưu Hóa Vi Kiến Trúc & Tập Lệnh SIMD**:
   - Bạn có ý tưởng đẩy tốc độ quét tia $0.34\text{ ns}$ nhanh hơn nữa trên ARM NEON, AVX-512, hoặc RISC-V Vector Extensions? Mình rất nóng lòng muốn được chiêm ngưỡng đoạn mã Assembly của bạn!
3. **Thực Nghiệm Phần Cứng Nhúng Đa Nền Tảng**:
   - Giúp mình kiểm thử và đo đạc trên các bo mạch phần cứng giá rẻ: STM32H7, ESP32-S3, Raspberry Pi CM4, NVIDIA Jetson Orin Nano, hoặc vi điều khiển RISC-V.
4. **Thuật Toán Tìm Đường Bất Kỳ Góc & Động Học Bay**:
   - Cải tiến thuật toán kéo căng dây SSFA, làm mượt góc bẻ lái Spline, hoặc bổ sung bù trừ gió dạt.
5. **Dọn Dẹp & Chuẩn Hóa Mã Nguồn (Refactoring)**:
   - Thấy chỗ nào đặt tên biến khó hiểu hay bình luận còn "ngô nghê"? Những đóng góp giúp code sáng sủa, mạch lạc luôn được hoan nghênh nồng nhiệt!

---

## 📬 Quy Trình Gửi Pull Request (PR)

1. **Mở Issue Trước**:
   - Thảo luận về ý tưởng tính năng hoặc thay đổi bạn muốn thực hiện trước khi bắt tay viết một khối lượng code lớn.
2. **Tạo Nhánh Mới (Feature Branch)**:
   ```bash
   git checkout -b feature/toi-uu-cuc-chien
   ```
3. **Chạy Kiểm Thử Độc Lập 11 Giai Đoạn**:
   - Trước khi gửi PR, hãy chạy toàn bộ pipeline kiểm định:
     ```bash
     ./scripts/build_and_verify.sh
     ```
   - **Tất cả 11 giai đoạn phải đạt chuẩn 100%**:
     - Giai đoạn 1: Chuẩn định dạng Clang-Format (100% tuân thủ `.clang-format`).
     - Giai đoạn 2: Phân tích tĩnh Clang-Tidy (0 rủi ro an toàn bộ nhớ, 0 lỗi logic ngầm).
     - Giai đoạn 3: Kiểm toán mã máy Assembly (0 byte cấp phát động heap trong vòng lặp intrinsic).
     - Giai đoạn 4: ASan & UBSan sạch bong (0 rò rỉ bộ nhớ, 0 hành vi bất định).
     - Giai đoạn 5: Kích thước file nhị phân stripped $< 40\text{ KB}$ ($34,304\text{ bytes}$).
     - Giai đoạn 6: Mô phỏng bay né 500 vật cản đạt đúng 0.00% va chạm qua 5.000 chu kỳ.
     - Giai đoạn 7: Cổng kiểm thử vi kiến trúc ($< 0.35\text{ ns}$ raycast, P99 $< 500\text{ ns}$ JPS+).
     - Giai đoạn 8: Định tuyến không gian đô thị & đại lục với tổng RAM $\le 16.00\text{ MB}$.
     - Giai đoạn 9: Kiểm thử vi điều khiển & ESP32 Zero-Heap tĩnh (100% xác thực trên 64 KB SRAM).
     - Giai đoạn 10: Project Omni-Aegis Universal Genius Benchmark (4 cổng kiểm thử vật lý: nạp cảm biến $< 10\ \mu\text{s}$, động học tuyến $< 3\ \mu\text{s}$, bộ nhớ micro $\le 64\text{ KB}$, 0.00% va chạm vật cản động, bộ điều khiển bám 1 kHz $< 50\text{ ns}$).
     - Giai đoạn 11: Bộ kiểm chuẩn chuẩn công nghiệp Google Benchmark (`halo_google_benchmark.cpp`).
4. **Gửi Pull Request**:
   - Mô tả ngắn gọn, súc tích điều bạn đã thay đổi, lý do thay đổi và số liệu đo đạc so sánh trước/sau.
5. **Chờ Phản Hồi Review**:
   - Mình sẽ cố gắng review PR của bạn sớm nhất có thể ngay sau khi hoàn thành bài tập về nhà hoặc các đợt thi cử ở trường! 🥤

---

## ❤️ Lời Kết

Mình vô cùng biết ơn thời gian, trí tuệ và sự đồng hành của bạn. Cùng nhau, chúng ta sẽ kiến tạo nên lõi điều hướng robot mã nguồn mở nhanh nhất, tin cậy nhất thế giới — cứu sống từng sinh mạng bằng từng nanosecond quý giá.

> *"Stay hungry, stay humble, and keep optimizing."* 🚀✨🌱
