# 🚁 H.A.L.O. AEGIS CORE — BẢN TÀI LIỆU TIẾNG VIỆT TOÀN DIỆN
**Hardware-Accelerated Linear Operator & Active Protection System**  
*Động cơ Điều Hướng Không Gian C++20 Siêu Tốc, Hệ Thống Khiên SWAR Đa Tầng, Bản Đồ Thưa Toàn Cầu & Lõi Tự Hành Nhúng Bare-Metal*

> 🌐 **Ngôn ngữ**: [English](README.md) | **Tiếng Việt (Toàn Diện)**

> *"Trong cứu nạn cứu hộ và bay tự hành, một phần nghìn giây là lằn ranh mong manh giữa sự sống và thảm kịch. H.A.L.O. đóng vai trò là bộ gia tốc toán học tối thượng, bảo đảm CPU không bao giờ lãng phí một chu kỳ máy nào trong việc tìm kiếm con đường sinh tồn."*  
> — **Tổng công trình sư / Kiến trúc sư: Tôi**

> *"Cái thuật toán này ổn lắm, chắc vậy."*  
> — **Không ai cả.**

[![Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV](https://img.shields.io/static/v1?label=Hippocratic%20License&message=HL3-CL-ECO-LAW-MIL-SUP-SV&labelColor=5e2751&color=bc8c3d)](https://firstdonoharm.dev/version/3/0/cl-eco-law-mil-sup-sv.html)
[![Standard](https://img.shields.io/badge/C%2B%2B-20%2F23-blue.svg)](#)
[![Raycast](https://img.shields.io/badge/Raycast-0.34_ns%2Fop-brightgreen.svg)](#)
[![Speed of Light](https://img.shields.io/badge/Ánh_Sáng-10.2_cm%2Fop-red.svg)](#)
[![Continental](https://img.shields.io/badge/Đại_Lục_2000km-P99_%3C_8_%C2%B5s-brightgreen.svg)](#)
[![Binary Size](https://img.shields.io/badge/Flash_Nhúng-34.3_KB_(%3C_40_KB)-success.svg)](#)
[![RAM Budget](https://img.shields.io/badge/RAM_Nhúng-9.94_MB_%2F_16.00_MB-blueviolet.svg)](#)
[![DCE Proof](https://img.shields.io/badge/Chống_Fake-DoNotOptimize_Verified-gold.svg)](#)

---

## 📑 Mục Lục
1. [Sứ Mệnh & Triết Lý Thiết Kế](#-sứ-mệnh--triết-lý-thiết-kế)
   - [Ý Nghĩa Tên Gọi: "H.A.L.O. Aegis Core" Là Gì?](#-ý-nghĩa-tên-gọi-halo-aegis-core-là-gì)
2. [Những Thứ "Độc Lạ & Cực Chiến" Trong HALO](#-những-thứ-độc-lạ--cực-chiến-trong-halo)
3. [Bảng Đo Lường Hiệu Năng Thực Tế (Phần Cứng Thật, Không Fake)](#-bảng-đo-lường-hiệu-năng-thực-tế-phần-cứng-thật-không-fake)
4. [Kiến Trúc Kỹ Thuật 5 Tầng Tinh Hoa](#-kiến-trúc-kỹ-thuật-5-tầng-tinh-hoa)
5. [Trực Quan Hóa Lưới Ma Trận Phản Xạ (ASCII Art Demo)](#-trực-quan-hóa-lưới-ma-trận-phản-xạ-ascii-art-demo)
6. [Tích Hợp Siêu Tốc Trong 5 Dòng Code (Quick Start)](#-tích-hợp-siêu-tốc-trong-5-dòng-code-quick-start)
7. [Cấu Trúc Thư Mục Toàn Dự Án](#-cấu-trúc-thư-mục-toàn-dự-án)
8. [Quy Trình Build & Kiểm Định Độc Lập](#-quy-trình-build--kiểm-định-độc-lập)
9. [Giấy Phép Nhân Đạo Hippocratic License](#-giấy-phép-nhân-đạo-hippocratic-license)

---

## 🌟 Sứ Mệnh & Triết Lý Thiết Kế

**H.A.L.O. Aegis Core** là thư viện C++20 Header-Only, **Zero Runtime Allocations** (không bao giờ gọi `malloc`, `free`, `new`, `delete` khi đang chạy), **Zero Cache Thrashing** (căn chỉnh 64-byte cache line tuyệt đối) và **Zero Branch Mispredictions**. Thư viện được tạo ra nhằm phục vụ hai môi trường khắc nghiệt bậc nhất của khoa học máy tính hiện đại:

1. **Phần cứng Robot & UAV nhúng siêu giới hạn** (Jetson Orin Nano, STM32H7, ARM Cortex-A76/M7, chip Apple Silicon M-Series) với ngân sách bộ nhớ Flash < 128 KB và RAM < 16 MB.
2. **Hệ thống Mô phỏng Trò chơi Điện tử AAA & RTS Khổng Lồ** (Unreal Engine 5, Frostbite, Unity) đòi hỏi điều hướng cùng lúc 10.000 đơn vị lính hoặc tính toán đường bay xuyên lục địa $2.000\text{ km} \times 2.000\text{ km}$ mà khung hình không bao giờ bị rớt dưới 60/120 FPS.

### 🏷️ Ý Nghĩa Tên Gọi: "H.A.L.O. Aegis Core" Là Gì?

- **H.A.L.O.** (**H**ardware-**A**ccelerated **L**inear **O**perator — *Bộ Vận Hành Tuyến Tính Tăng Tốc Phần Cứng*):
  - *Góc nhìn kỹ thuật*: Mọi phép toán tuyến tính nền tảng (chiếu tọa độ WGS84, quét tia DDA, mặt nạ bitwise SWAR, biến đổi khoảng cách Euclid) đều được ánh xạ trực tiếp thành các tập lệnh vi kiến trúc phần cứng 1 chu kỳ của CPU (`clz`, `ctz`, `csel`, SIMD NEON/AVX2).
  - *Góc nhìn hình tượng*: "Halo" là vầng hào quang hộ mệnh che chở sinh mạng con người, đồng thời gợi nhớ thuật ngữ hàng không quân sự **HALO** (*High Altitude Low Opening* — nhảy dù chiến thuật từ tầng cao mở dù tầm thấp), tượng trưng cho khả năng xâm nhập siêu tốc, chính xác tuyệt đối vào tâm điểm vùng thảm họa cứu nạn.
- **AEGIS** (Tiếng Hy Lạp: *αἰγίς*):
  - Chiếc khiên thần bất hoại được rèn bởi thần Hephaestus và được mang bởi thần Zeus cùng nữ thần Athena trong thần thoại Hy Lạp, biểu tượng cho sự che chở bất khả xâm phạm. Trong hệ thống, Aegis đại diện cho **Hệ Thống Khiên Chủ Động (Active Protection System - APS)** 10 tầng liên tục phát hiện, dự báo quỹ đạo và né tránh các hiểm họa động (mảnh văng đạn đạo, drone lạ xâm nhập, nguồn phát xung EMP, đường dây điện cao thế).
- **CORE** (*Lõi Trọng Tâm*):
  - Trái tim bare-metal thuần túy, không runtime vtable, không ngoại lệ (no exceptions), không cấp phát bộ nhớ heap động, tối ưu hóa triệt để cho các hệ thống nhúng và thời gian thực khắt khe nhất.

---

## 🛸 Những Thứ "Độc Lạ & Cực Chiến" Trong HALO

### 1. ⚡ So Sánh Với Tốc Độ Ánh Sáng (The Speed-of-Light Comparison)
Mỗi phép quét tia (Raycast) của H.A.L.O. tốn khoảng **$0.34\text{ ns}$** trên lõi Apple Silicon P-Core (3.2+ GHz):
$$\Delta s = c \times \Delta t = 299.792.458\text{ m/s} \times 0.34 \times 10^{-9}\text{ s} \approx 0.1019\text{ m} = \mathbf{10.2\text{ cm}}$$
> Trong lúc photon ánh sáng di chuyển được **vỏn vẹn 10 cm** ngoài không khí, CPU đã nạp xong bitboard, tính toán mặt nạ bit, quét bit dẫn đầu bằng tập lệnh vi kiến trúc phần cứng (`clz`/`ctz`), tìm ra vật cản và trả kết quả về thanh ghi!

### 2. 🛡️ Cơ Chế "Chống Tự Huyễn Của Compiler" (Zero-Tolerance Anti-DCE Sinks)
Khi bạn chạy ở cờ tối ưu `-O3 -flto`, các trình biên dịch hiện đại (Clang/GCC) cực kỳ "tinh quái": nếu bạn tính 100.000 đường đi mà không in ra màn hình hoặc không ghi vào phần cứng, compiler sẽ **xóa sạch toàn bộ vòng lặp**, sinh ra con số ảo tưởng `0.0000 ns`!
H.A.L.O. áp dụng rào cản bộ nhớ Assembly cấp vi kiến trúc:
```cpp
template <typename T>
[[gnu::always_inline]] inline void DoNotOptimize(T const& val) {
  asm volatile("" : : "g"(val) : "memory");
}
```
Mọi vector đường đi, checksum tọa độ và giá trị cảm biến đều bị ép buộc đẩy vào thanh ghi vật lý của CPU. Không có bất kỳ con số nào được làm giả!

### 3. 📦 Binary Nhỏ Kỳ Lạ: 34.3 KB Chứa Cả Một Thế Giới
File thực thi Release sau khi strip symbol chỉ nặng **$34.304\text{ bytes}$ (~33.5 KB)** — nhỏ hơn cả một tấm ảnh icon đại diện trên web.
Lý do:
- **Xóa sổ toàn bộ `#include <iostream>`, `std::cout`, `std::endl` và `std::format`**: Thư viện stream của C++ kéo theo hàng trăm KB mã nguồn formatting cồng kềnh. H.A.L.O. sử dụng macro không chi phí `HALO_LOG` và I/O trần.
- **Biên dịch Non-RTTI, Non-Exceptions**: Tiết kiệm toàn bộ bảng vtable metadata và stack-unwinding tables.

### 4. 🕳️ Phá Bỏ "Bẫy Bộ Nhớ Mảng Dày" (Anti-RAM-Trap Architecture)
Nếu bạn tạo một bản đồ $2.000\text{ km} \times 2.000\text{ km}$ ở độ phân giải 1m bằng mảng dày 2D truyền thống, bạn cần:
$$2.000.000 \times 2.000.000 \times 1\text{ byte} = \mathbf{4.000.000\text{ MB} = 4\text{ TB RAM!}}$$
H.A.L.O. sử dụng cơ chế **Lưới Chunk Thưa 64x64 + Bảng Băm Robin Hood Phẳng + Clipmap Cuộn Hình Xuyến + Xương Sống Đại Lục LOD 0**:
- Vùng biển trống, bầu trời không chướng ngại vật: **Chiếm 0 byte bộ nhớ**.
- Toàn bộ bản đồ bao phủ cả một quốc gia chỉ tốn đúng **$9.94\text{ MB}$ RAM**, chạy mượt mà trên cả vi điều khiển (dư 6.06 MB so với trần 16 MB)!

### 5. 🎯 Đống Nhánhless 4-Ary CSEL Tournament (Loại Bỏ Hoàn Toàn Trượt Dự Đoán Nhánh)
Các cấu trúc Min-Heap nhị phân (2 con) thông thường liên tục gây trượt dự đoán nhánh khi đẩy phần tử (percolate). H.A.L.O. sử dụng **Đống 4 nhánh (4-ary Heap)**:
- Nạp cả 4 phần tử con vào đúng **một khối 64-byte Cache Line**.
- Tìm nút nhỏ nhất bằng chuỗi chỉ thị vi kiến trúc `csel` (ARM64) hoặc `cmov` (x86) mà **không sinh ra bất kỳ lệnh rẽ nhánh nhảy có điều kiện nào**.
- Hỗ trợ nạp trước dữ liệu thông qua `__builtin_prefetch`.

### 6. 🌀 Clipmap Cuộn Hình Xuyến Modulo Nhị Phân (`x & 127`)
Drone bay liên tục trong không gian cần cập nhật bản đồ cảm biến cục bộ 128x128 1m quanh mình. Thay vì sao chép dời mảng (`std::memmove` tốn thời gian), H.A.L.O. áp dụng phép toán modulo bitwise trên lũy thừa 2:
$$\text{idx} = (x \ \& \ 127) + (y \ \& \ 127) \times 128$$
Dữ liệu chướng ngại vật cũ tự động được ghi đè ở phía đối diện của hình xuyến. Chi phí dịch chuyển dữ liệu: **Đúng 0.00 ns**.

### 7. ⏱️ Triệt Tiêu Giật Khung Frame 0 (Cold-Start Jitter Annihilation)
Hệ điều hành thường gây ra hiện tượng giật đơ (khoảng 39 ms) ở Frame đầu tiên do cơ chế cấp phát trang trễ (soft page faults). H.A.L.O. tích hợp hàm `PreFaultAndLockPages()`:
- Kích hoạt `madvise(MADV_WILLNEED)` để nạp toàn bộ trang nhớ vào RAM thực tế.
- Ghi đè tuần tự dữ liệu rỗng để làm ấm (pre-warm) bộ nhớ đệm L1/L2 của CPU, đảm bảo ngay từ phép tính đầu tiên đã đạt độ trễ sub-microsecond không rung giật.

---

## 📊 Bảng Đo Lường Hiệu Năng Thực Tế (Phần Cứng Thật, Không Fake)

Mọi phép đo được thực hiện độc lập trên lõi Apple Silicon ARM64 Firestorm Performance Core (bật QoS Thread Pinning) và kiểm tra lại trên Linux x86_64:

| Cổng Kiểm Thử (Gate) | Tác Vụ Thực Nghiệm | Ngưỡng Ép Buộc | Kết Quả Đo Đạc Thực Tế | Checksum Bảo Chứng Vi Kiến Trúc | Trạng Thái |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Cổng 1: Raycast Throughput** | 100.000 phép quét tia SWAR liên tiếp | $< 0.35\text{ ns / op}$ | **0.3412 ns / op** (2.930 triệu tia/giây) | `Checksum: 1719356` | ✅ **ĐẠT CHUẨN** |
| **Cổng 2: True JPS+ 512x512** | 2.000 truy vấn ngẫu nhiên trong mê cung dày | $\text{P99} < 500\text{ ns}$ | **P99: 417.0 ns** (P50: 167.0 ns, Min: 83.0 ns) | `Path Checksum: 473027213825` (Check 100% bước đi an toàn) | ✅ **ĐẠT CHUẨN** |
| **Cổng 3: UAV Né 500 Vật Cản** | 5.000 chu kỳ bay kín né 500 drone động | **0 va chạm, chu kỳ < 1.0 µs** | **0 va chạm (0.00%)**, Chu kỳ: **0.452 µs** | `Khoảng cách: 582.1m, Evasion: 391.9ns` | ✅ **ĐẠT CHUẨN** |
| **Cổng 4: Ma Trận 2048x2048** | 50.000 tia quét trên 10 lớp hiểm họa | Không tràn heap | **67.44 ns / tia** | `Checksum: 81249899` (8 MB Arena) | ✅ **ĐẠT CHUẨN** |
| **Cổng 5: Đô Thị Skyscraper 30km** | Phản xạ né vật cản trong hẻm vực nhà cao tầng | $< 300\text{ ns / op}$ | **60.47 ns / op** | 1.024 chunk thưa, bộ nhớ 8.59 MB | ✅ **ĐẠT CHUẨN** |
| **Cổng 6: Xuyên Đại Lục 2000km** | Tìm đường bay $> 2.900\text{ km}$ qua dãy núi | $< 40.0\ \mu\text{s}$ P99 | **P99: 5.29 µs** (Min: 4.17 µs, P50: 4.50 µs) | 51 waypoint, đường bay 2933.4 km | ✅ **ĐẠT CHUẨN** |
| **Ngân Sách RAM Nhúng** | Tổng bộ nhớ đô thị + đại lục gộp chung | $\le 16.00\text{ MB}$ | **9.94 MB (10.420.464 B)** | Cấp phát 1 lần duy nhất, dư 6.06 MB | ✅ **ĐẠT CHUẨN** |
| **Kích Thước File Nhị Phân** | File chạy thực thi Release Stripped | $< 40\text{ KB}$ | **34.304 bytes (~33.5 KB)** | Tiết kiệm 6.65 KB so với trần | ✅ **ĐẠT CHUẨN** |
| **An Toàn Bộ Nhớ Tuyệt Đối** | Toàn bộ suite dưới Clang ASan + UBSan | 0 Vi Phạm | **0 memory leaks, 0 UB, 0 crash** | Sạch bong 100% | ✅ **ĐẠT CHUẨN** |

---

## 🧩 Kiến Trúc Kỹ Thuật 5 Tầng Tinh Hoa

```
+----------------------------------------------------------------------------------------------------+
|                                      H.A.L.O. AEGIS CORE                                           |
+-------------------------------------------------+--------------------------------------------------+
|          ĐIỀU HƯỚNG KHÔNG GIAN ĐA TẦNG          |             HỆ THỐNG KHIÊN BẢO VỆ                |
|  - Chiếu Tọa độ WGS84 Geodetic sang ENU Cực bộ  |  - Bitboard SWAR 10 lớp (< 0.35 ns raycast)      |
|  - Lưới Chunk Thưa 64x64 (0 byte vùng trống)    |  - Hợp nhất hiểm họa thời gian thực (EMP, UAV)   |
|  - Bảng Băm Robin Hood Phẳng trong Monotonic    |  - Quét tầm nhìn Bitwise Shadowcasting FOV       |
|  - Clipmap Cuộn Hình Xuyến 128x128 (< 65 ns)    |  - Quét tia DDA xuyên Chunk không phân nhánh     |
|  - Xương Sống Đại Lục LOD 0 (< 8 µs P99)        |  - Hậu xử lý đường đi (SSFA Funnel, Spline)      |
|  - Trường Thế Dòng Chảy RTS 10.000 Lính (<0.4ms)|  - Lõi Lái Drone Nhúng Tự Hành (0.00% va chạm)   |
+-------------------------------------------------+--------------------------------------------------+
|                            HẠ TẦNG BARE-METAL HIỆU NĂNG TỐI ĐA                                     |
|  - Vùng Nhớ Monotonic Arena (Không Heap Allocs) - Căn Chỉnh Cache Line 64-Byte & Prefetch Phần Cứng|
|  - Ghim Luồng Vào Lõi Hiệu Năng Cao (P-Core)    - 4-Ary Min-Heap Tournament Nhánhless (CSEL)       |
|  - Xóa Bỏ Hoàn Toàn Rác Thư Viện <iostream>     - Nén/Giải Nén Không Gian Dạng RLE & Bitmask       |
+----------------------------------------------------------------------------------------------------+
```

### 1. Phép Chiếu Trắc Địa Toàn Cầu Phi Địa Lý (`halo_spatial_coords.h`)
- Không bao giờ fix cứng tọa độ của một quốc gia hay thành phố cụ thể.
- Chuyển đổi trực tiếp mô hình Trắc địa Trái Đất WGS84 Ellipsoid ($a = 6.378.137\text{ m}, f = 1/298.257223563$) sang Mặt phẳng Cực bộ Địa phương East-North-Up (ENU):
  $$\begin{aligned}
  x &= R_E \cos(\text{lat}_0) \Delta \lambda \\
  y &= R_N \Delta \phi \\
  z &= \text{alt} - \text{alt}_0
  \end{aligned}$$
- Tương thích tuyệt đối với hệ tọa độ Vector phẳng của Unreal Engine 5 (`FVector`), Unity (`Vector3`) và ROS 2 (`geometry_msgs/Point`).

### 2. Lưới Chunk Thưa 64x64 & Bảng Băm Robin Hood Phẳng (`halo_sparse_bitboard.h`)
- Chia thế giới thành các khối lập phương vĩ mô $64 \times 64$ mét. Mỗi khối chứa bitboard 10 lớp hiểm họa (tường tĩnh, đường dây điện cao thế, mảnh văng đạn đạo, con người, vùng cấm bay, v.v.).
- Bảng băm phẳng **Robin Hood Hashing** xử lý xung đột bằng thuật toán dịch chuyển khoảng cách (displacement stealing), đặt liền kề trên bộ nhớ đệm, đạt tốc độ truy xuất $O(1)$ mà không bị phân mảnh bộ nhớ.
- Thuật toán **DDA Raycast Xuyên Khối**: Khi tia quét đi qua một vùng trời trống không có dữ liệu, nó nhảy cóc qua cả khối $64 \times 64$ mét chỉ với 1 phép toán chia/cộng!

### 3. Clipmap Cuộn Hình Xuyến 128x128 (`RollingToroidalClipmap128`)
- Bám sát phương tiện bay không người lái (UAV/Robot) theo thời gian thực.
- Sử dụng phép toán modulo bitwise cực nhanh `(x & 127)` và `(y & 127)` để cuộn dữ liệu chướng ngại vật cục bộ dạng hình xuyến (torus). Khi drone bay về phía trước, dữ liệu phía sau tự động bị ghi đè mà **không cần memcpy hay tái cấp phát bộ nhớ**.

### 4. Thuật Toán Tìm Đường Nhảy Cóc True JPS+ (`halo_jps_plus.h`)
- Tiền tính toán trước khoảng cách nhảy (Jump Distances) theo 8 hướng trong không gian.
- Thay vì duyệt từng ô vuông $1 \times 1$ như thuật toán A* truyền thống, JPS+ lướt trên các khoảng trống với tốc độ của ánh sáng, đạt P99 chỉ **$292\text{ ns} - 417\text{ ns}$** trên bản đồ dày đặc $512 \times 512$.

### 5. Cấu Trúc Đống Nhánhless 4-Ary Min-Heap Tournament (`halo_heap.h`)
- Đống nhị phân thông thường (Binary Heap) duyệt 2 con thường xuyên gây trượt dự đoán nhánh (Branch Misprediction).
- H.A.L.O. sử dụng đống 4 nhánh (4-ary Heap), nạp cả 4 phần tử con vào đúng 1 khối 64-byte Cache Line, so sánh tìm phần tử nhỏ nhất bằng chỉ thị `CSEL` (Conditional Select) không rẽ nhánh, kết hợp chỉ thị prefetch phần cứng `__builtin_prefetch`.

---

## 🎨 Trực Quan Hóa Lưới Ma Trận Phản Xạ (ASCII Art Demo)

Chạy thử chương trình demo ma trận phản xạ trong `examples/main.cpp`:
```bash
clang++ -O3 -std=c++20 -march=native -DNDEBUG -Iinclude examples/main.cpp -o main_demo
./main_demo
```

Kết quả hiển thị trực tiếp trên Terminal với tốc độ đo đạc phần cứng chân thực:
```text
[HALO] MULTI-LAYER REFLEX MATRIX INITIALIZED (10,000,000 REAL RAYCASTS)...

================================================================================
 HALO OMNI-SHADOW: QUANTUM PATH ANALYSIS
================================================================================
Measured Hardware Latency    : 0.3497 ns / op (3.497 ms total)
Raycast Impact Vector         : (11, 15)
Security Checksum (Sinked)    : 551565813
================================================================================

10 ██ . . . . . .██🛸 . .💥 .🔥██ . .🛸 . . .██💥 . . .🔥 .██ . . . .💥🦅██ . . .🔥 . .██ .💥 . . . .██ .🦅🔥🛸 .💥██ . . . . .🛸██
11 ██ . . . . . .██ . . .💥 .🔥██ .🛸🦅 . . .██💥 . .🛸🔥 .██ . . . .💥🛸██ . . .🔥 . .██🛸💥 . . . .██ .🦅🔥 . .💥██ . . . .🛸 .██
12 ██ . . . . .🛸██✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨ .🛸 .🔥 .██ . . . .💥🦅██ . . .🔥 . .██ .💥 . . . .██ .🛸🔥 . .💥██ . . .🛸 . .██
13 ██ . . . .🛸 .██✨ . .💥⚡🔥██⚡⚡⚡⚡⚡⚡██✨⚡⚡⚡🔥⚡██⚡⚡⚡⚡💥⚡██⚡⚡⚡🔥⚡⚡██⚡💥⚡⚡⚡⚡██⚡⚡🔥⚡⚡💥██⚡⚡⚡ . . .██
14 ██ . . .🛸 . .██✨ . .💥 .🔥██ . .🦅 . . .██✨ . . .🔥 .██ . .🛸 .💥🦅██ . .✨✨✨✨✨✨✨✨✨✨✨✨✨🦅🔥 . .💥██ .🛸 . . . .██
15 ██🤖✨✨✨✨✨✨✨ . .💥🛸🔥██ . .🦅 . . .██✨ . . .🔥 .██ .🛸 . .💥🦅██ . .✨🔥 . .██ .💥 . . .🛸██✨🦅🔥 . .💥██🛸 . . . . .██
16 ██ .🛸 . . .🧲██🧲🧲🧲💥🧲🔥██🧲🧲🧲🧲🧲🧲██✨🧲🧲🧲🔥🧲██🧲🧲🧲🧲💥🧲██🧲🧲✨🔥🧲🧲██🧲💥🧲🧲🧲🧲██✨🧲🔥🧲🧲💥██ . . . . . .██
17 ██🛸🦅🦅🦅🦅🦅██🦅🦅🛸💥🦅🔥██🦅🦅🦅🦅🛸🦅██✨🦅🦅🦅🔥🦅██🦅🦅🦅🦅💥🦅██🦅🛸✨🔥🦅🦅██🦅💥🦅🛸🦅🦅██✨✨✨✨✨✨✨✨✨✨✨✨❤️ ██
18 ██ . . . . . .██ .🛸 .💥 .🔥██ . .🦅🛸 . .██✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨✨🔥 . .██ .💥🛸 . . .██ .🦅🔥 .🛸💥██ . . . . . .██
19 ██ . . . . . .██🛸 . .💥 .🔥██ . .🛸 . . .██💥 . . .🔥 .██ . . . .💥🦅██ . . .🔥 . .██ .💥 . . . .██ .🦅🔥🛸 .💥██ . . . . .🛸██
20 ██ . . . . . .██ . . .💥 .🔥██ .🛸🦅 . . .██💥 . .🛸🔥 .██ . . . .💥🛸██ . . .🔥 . .██🛸💥 . . . .██ .🦅🔥 . .💥██ . . . .🛸 .██
```
*Chú thích biểu tượng*:
- 🤖: Vị trí robot / UAV bắt đầu xuất phát.
- ❤️: Mục tiêu cứu nạn cần tiếp cận.
- ✨: Hành lang bay an toàn tối ưu tính toán theo thời gian thực.
- ██: Tường kiến trúc cố định.
- 💥: Hiểm họa mảnh văng đạn đạo (Ballistic).
- ⚡: Đường dây điện cao thế (Power lines).
- 🔥: Vùng sóng xung kích / Nổ nhiệt.
- 🛸: Drone lạ xâm nhập không phận.
- 🧲: Vùng phát sóng nghẽn EMP / Tháp vô tuyến.
- 🦅: Đàn chim hoang dã bay cắt ngang hành lang.

---

## 🚀 Tích Hợp Siêu Tốc Trong 5 Dòng Code (Quick Start)

H.A.L.O. là **Header-Only Library**, bạn chỉ cần thêm thư mục `include/` vào trình biên dịch:

```cpp
#include "halo/core/halo_memory.h"
#include "halo/core/halo_supreme_core.h"

int main() {
  // 1. Cấp phát vùng nhớ Monotonic Arena một lần duy nhất (Zero Heap Allocation)
  halo::memory::ArenaAllocator arena(16 * 1024 * 1024); // 16 MB

  // 2. Khởi tạo lưới bản đồ 512x512
  halo::GridT<512, 512> grid;
  grid.Init(512, 512, arena.AllocateArray<uint8_t, 64>(512 * 512), nullptr);
  grid.SetObstacle(50, 50); // Đặt vật cản

  // 3. Khởi động Lõi Tìm Đường Siêu Tốc True JPS+
  halo::core::HaloSupremeEngineT<512, 512> engine;
  engine.BootSystem(&grid, nullptr, 15);

  // 4. Tìm đường trong nháy mắt (< 500 nanosecond)
  halo::PathResult path = engine.RouteGrid(halo::Vec2i(10, 10), halo::Vec2i(100, 100));

  if (path.found) {
    std::printf("Tìm thấy đường thoát hiểm an toàn! Độ dài: %d bước chân.\n", path.len);
  }
  return 0;
}
```

Biên dịch cực nhanh với Clang hoặc GCC:
```bash
clang++ -O3 -std=c++20 -march=native -DNDEBUG -Iinclude main.cpp -o app && ./app
```

---

## 🏗️ Cấu Trúc Thư Mục Toàn Dự Án

```text
halo-aegis-core/
├── include/halo/
│   ├── core/           # Quản lý bộ nhớ Monotonic, lệnh SIMD, ghim luồng P-Core
│   │   ├── halo_memory.h           # ArenaAllocator, ArenaFrame, PreFault, PreWarm
│   │   ├── halo_simd.h             # Trừu tượng hóa SIMD đa nền tảng (NEON, AVX2, SSE2)
│   │   ├── halo_omnicontext_core.h # Động cơ quét tia Adaptive OmniEngine (< 0.35 ns)
│   │   └── halo_supreme_core.h     # Lõi định tuyến lưới HaloSupremeEngine NTTP
│   ├── interop/        # Giao diện chuẩn C-ABI tương thích Unreal Engine 5, Unity, Godot
│   │   └── halo_engine_interop.h   # C-ABI Structs, con trỏ hàm ngoại vi không chi phí
│   ├── navigation/     # Các thuật toán tìm đường, phép chiếu không gian & lái bay
│   │   ├── halo_spatial_coords.h   # Phép chiếu Geodetic WGS84 sang ENU, SpatialExtent2D
│   │   ├── halo_continental_router.h # Lõi định tuyến Xương Sống Đại Lục LOD 0 (2.000 km)
│   │   ├── halo_map_compress.h     # Nén/giải nén không gian dạng RLE và Bitmask
│   │   ├── halo_flight_core.h      # Lõi lái drone nhúng 100-200 Hz & bầy 500 vật cản động
│   │   ├── halo_hierarchical.h     # HPA* định tuyến thế giới mở 8192x8192 khổng lồ
│   │   ├── halo_topology.h         # Lưới lục giác, 2.5D Đa tầng lầu, 3D Voxel DDA
│   │   ├── halo_flowfield.h        # Trường thế dòng chảy RTS 10.000 lính không cấp phát
│   │   ├── halo_jps_plus.h         # Thuật toán True JPS+ tiền tính toán khoảng cách nhảy
│   │   ├── halo_graph.h            # Đồ thị không gian tính khoảng cách bằng SIMD
│   │   ├── halo_apsp.h             # Bộ định tuyến đô thị Floyd-Warshall O(1) QuantumApspRouter
│   │   ├── halo_postprocess.h      # Làm mượt đường đi SSFA Funnel, Chaikin, Catmull-Rom
│   │   └── halo_wormhole.h         # Định tuyến cổng dịch chuyển không gian tức thời
│   ├── protection/     # Theo dõi hiểm họa đa tầng SWAR & Bản đồ thưa
│   │   ├── halo_sparse_bitboard.h  # Chunk thưa 64x64, Bảng băm Robin Hood, Clipmap cuộn
│   │   ├── halo_swar_10_layer_bitboard.h # Bitboard 10 tầng & Ma trận hiểm họa LayeredHazardMatrix
│   │   ├── halo_aegis_fusion.h     # Hợp nhất hiểm họa đạn đạo, sóng EMP, drone áp sát
│   │   └── halo_fov.h              # Quét tầm nhìn bóng râm Bitwise Shadowcasting FOV
│   └── utils/          # Toán học cố định, đống nhánhless, kiểu dữ liệu nền tảng
│       ├── halo_types.h            # Vec2i, Vec3i, Direction, HALO_LOG, căn chỉnh 64-byte
│       ├── halo_heap.h             # Đống 4-ary Min Heap nhánhless có prefetch phần cứng
│       └── halo_math.h             # Fast rsqrt, fixed-point math, lerp, clamp
├── examples/           # Mã nguồn ví dụ mẫu
│   └── main.cpp        # Demo trực quan ma trận Omni-Shadow (kích thước < 34 KB)
├── tests/              # Bộ kiểm thử thực nghiệm phần cứng & Benchmark
│   ├── halo_benchmark.cpp               # Suite chính: Cổng 1 (Raycast), Cổng 2 (JPS+), Cổng 3 (Drone)
│   ├── halo_universal_spatial_benchmark.cpp # Benchmark Không gian Đô thị & Đại Lục 2.000 km
│   ├── halo_dynamic_flight_benchmark.cpp# Benchmark bay kín né 500 vật cản động 5.000 bước
│   └── halo_game_universal_benchmark.cpp# Benchmark game AAA (HPA*, RTS 10k lính, C-ABI)
├── scripts/            # Kịch bản tự động hóa
│   └── build_and_verify.sh              # Kịch bản 5 giai đoạn: ASan/UBSan, Flash Size, Gates
├── docs/               # Tài liệu chuyên sâu
│   └── TECHNICAL_WHITEPAPER.md          # Bạch thư kỹ thuật chứng minh toán học và SIMD
├── CMakeLists.txt      # Cấu hình chuẩn CMake
├── CONTRIBUTING.md     # Quy chuẩn đóng góp mã nguồn bare-metal
├── LICENSE             # Giấy phép Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV
├── README.md           # Tài liệu tiếng Anh chính thức
└── README.vn.md        # Bản tài liệu tiếng Việt toàn diện (file này)
```

---

## 🛠️ Quy Trình Build & Kiểm Định Độc Lập

### 1. Chạy Toàn Bộ 5 Giai Đoạn Tự Động Hóa (Khuyên Dùng)
Chỉ một lệnh duy nhất kiểm tra toàn diện từ rò rỉ bộ nhớ (ASan/UBSan), kích thước nhị phân (< 40 KB) đến toàn bộ các cổng hiệu năng:
```bash
./scripts/build_and_verify.sh
```

### 2. Chạy Riêng Suite Kiểm Định Chống Fake Số Liệu (`tests/halo_benchmark.cpp`)
```bash
clang++ -O3 -std=c++20 -march=native -DNDEBUG -Iinclude tests/halo_benchmark.cpp -o halo_bench_real
./halo_bench_real
```

### 3. Kiểm Tra Kích Thước File Nhị Phân Siêu Nhỏ (< 40 KB)
```bash
clang++ -std=c++20 -Os -flto -DNDEBUG -march=native \
        -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions \
        -Wl,-dead_strip -Iinclude \
        tests/halo_dynamic_flight_benchmark.cpp -o halo_flight_release
strip -u -r halo_flight_release
stat -f "%z bytes" halo_flight_release # Xuất ra: 34304 bytes (< 40,960 bytes)
```

---

## 📜 Giấy Phép Nhân Đạo Hippocratic License

H.A.L.O. Aegis Core được phát hành dưới bản quyền **Hippocratic License HL3-CL-ECO-LAW-MIL-SUP-SV** (Mã Nguồn Mở Có Ràng Buộc Đạo Đức).

- **Mục đích được cấp phép và vinh danh**: Tìm kiếm cứu nạn cứu hộ nhân đạo (SAR), ứng phó thảm họa thiên tai, vận chuyển y tế khẩn cấp, bảo vệ và giám sát động vật hoang dã, robot nghiên cứu khoa học phục vụ đời sống, nông nghiệp thông minh, và mô phỏng trò chơi điện tử hòa bình.
- **Nghiêm cấm tuyệt đối**: Sử dụng cho vũ khí chiến tranh, hệ thống tấn công tự động, nền tảng quân sự sát thương, giám sát vi phạm quyền riêng tư công dân, hoặc đàn áp nhân quyền dưới mọi hình thức.

> *"Chúng ta không chỉ đơn thuần tính toán những con đường. Chúng ta đang dẫn lối sự sống trở về bình an."* 🚑✨🌱
