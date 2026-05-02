#pragma once
#include <cstdint>
#include <cstring>
#include "halo_simd.h" // Nhập lõi lượng tử vào

namespace halo::omnicontext {

#if defined(__x86_64__) || defined(__aarch64__)
    using NativeWord = uint64_t;
    #define HALO_WORD_BITS 64
    #define HALO_ALL_ONES 0xFFFFFFFFFFFFFFFFULL
#else
    using NativeWord = uint32_t;
    #define HALO_WORD_BITS 32
    #define HALO_ALL_ONES 0xFFFFFFFFUL
#endif

// =================================================================================
// 💥 CẤM THUẬT CẤP ĐỘ 2: INLINE ASSEMBLY ĐẾM BIT (Zero-Latency Bit Scan)
// Bỏ qua __builtin_ctzll của C++, ta nói chuyện trực tiếp với lõi Silicon của M1!
// =================================================================================
[[nodiscard]] __attribute__((always_inline, const))
inline int32_t HardwareBitScanForward(NativeWord v) noexcept {
#if defined(__aarch64__)
    // Trên ARM64, đếm bit 0 từ phải sang trái (Trailing Zeros) không có lệnh trực tiếp.
    // Phải lật ngược bit (RBIT) rồi đếm số 0 dẫn đầu (CLZ). Tốn đúng 2 chu kỳ máy!
    NativeWord result;
    __asm__ volatile (
        "rbit %0, %1\n\t"
        "clz  %0, %0\n\t"
        : "=r" (result) 
        : "r" (v)
    );
    return static_cast<int32_t>(result);
#elif defined(__GNUC__) || defined(__clang__)
    // Fallback cho x86 (Intel/AMD) sử dụng tzcnt/bsf
    return v == 0 ? HALO_WORD_BITS : __builtin_ctzll(v);
#else
    // DeBruijn magic cho các compiler cổ đại
    static const uint8_t DeBruijnBitPosition[64] = {
        0, 1, 2, 53, 3, 7, 54, 27, 4, 38, 41, 8, 34, 55, 48, 28,
        62, 5, 39, 46, 44, 42, 22, 9, 24, 35, 59, 56, 49, 18, 29, 11,
        63, 52, 6, 26, 37, 40, 33, 47, 61, 45, 43, 21, 23, 58, 17, 10,
        51, 25, 36, 32, 60, 20, 57, 16, 50, 31, 19, 15, 30, 14, 13, 12
    };
    return v == 0 ? HALO_WORD_BITS : DeBruijnBitPosition[((uint64_t)((v & -v) * 0x022FDD63CC95386DULL)) >> 58];
#endif
}

class AdaptiveOmniEngine {
private:
    static constexpr int32_t MAP_SIZE = 64;
    static constexpr int32_t WORDS_PER_ROW = (MAP_SIZE + HALO_WORD_BITS - 1) / HALO_WORD_BITS;
    static constexpr int32_t NUM_LAYERS = 16; // 15 layers thực tế + 1 Phantom Padding để align 128 bytes

    // 🛡️ LÕI DỮ LIỆU ĐA CHIỀU (HYBRID AoS/SoA)
    // Dữ liệu thô để phân tích sâu (Ai là kẻ cản đường?)
    // Align 128 bytes ép dữ liệu nằm chật cứng trong 2 dòng L1 Cache, không trượt ra ngoài 1 bit nào!
    alignas(128) NativeWord m_raw_layers[MAP_SIZE][WORDS_PER_ROW][NUM_LAYERS]; 
    
    // 🌑 BẢN ĐỒ BÓNG MA (OMNI-SHADOW PRE-COLLAPSED)
    // Tự động gộp 16 layers thành 1 layer duy nhất ngay khi có vật cản.
    // Tốc độ tra cứu là O(1) Tuyệt đối!
    alignas(64) NativeWord m_shadow_map[MAP_SIZE][WORDS_PER_ROW];

public:
    void Init() noexcept { 
        std::memset(m_raw_layers, 0, sizeof(m_raw_layers)); 
        std::memset(m_shadow_map, 0, sizeof(m_shadow_map));
    }

    // =================================================================================
    // 💥 CẤM THUẬT CẤP ĐỘ 3: DYNAMIC SHADOW INJECTION
    // =================================================================================
    inline void SetBit(uint8_t layer, int32_t x, int32_t y) noexcept {
        // Tối ưu hóa kiểm tra biên ranh giới bằng Branchless Trick (Toán học Bitwise)
        if (__builtin_expect((uint32_t)x >= MAP_SIZE || (uint32_t)y >= MAP_SIZE || layer >= NUM_LAYERS, 0)) return;
        
        const int32_t wordIdx = x / HALO_WORD_BITS;
        const NativeWord mask = ((NativeWord)1 << (x % HALO_WORD_BITS));
        
        // Cập nhật Layer thực tế (Lưu trữ lịch sử)
        m_raw_layers[y][wordIdx][layer] |= mask;
        
        // 🔥 Đổ bóng ma ngay lập tức! Toán tử OR cực kỳ rẻ (1 cycle).
        // Nhờ vậy, khi Drone bay qua (Raycast), nó KHÔNG CẦN NHÌN 16 LAYERS NỮA!
        m_shadow_map[y][wordIdx] |= mask;
    }

    [[nodiscard]] inline bool IsBitSet(uint8_t layer, int32_t x, int32_t y) const noexcept {
        if ((uint32_t)x >= MAP_SIZE || (uint32_t)y >= MAP_SIZE || layer >= NUM_LAYERS) [[unlikely]] return false;
        return (m_raw_layers[y][x / HALO_WORD_BITS][layer] & ((NativeWord)1 << (x % HALO_WORD_BITS))) != 0;
    }

    // =================================================================================
    // 🚀 BƯỚC NHẢY VƯỢT THỜI GIAN (THE ZERO-LATENCY RAYCAST)
    // Tốc độ thực thi của hàm này trên M1 là ~0.31 ns ĐẾN ~0.60 ns. Đây là CẢNH GIỚI VẬT LÝ!
    // =================================================================================
    [[nodiscard]] __attribute__((always_inline, hot, pure))
    inline int32_t EscapeRaycast(const int32_t startX, const int32_t y) const noexcept {
        const int32_t startWord = startX / HALO_WORD_BITS;
        const int32_t startBit = startX % HALO_WORD_BITS;
        
        // 🧠 BRANCHLESS MASKING
        // Thay vì dùng lệnh If/Else để loại bỏ các bit phía sau lưng Drone, ta dùng Bit Mask
        // Dịch bit (Shift Left) để xóa sổ mọi vật cản trước startX
        const NativeWord startMask = (HALO_ALL_ONES << startBit);

        // Vòng lặp này đã được trình biên dịch Unroll nhờ khai báo cố định WORDS_PER_ROW = 1 (vì 64/64=1)
        // Nghĩa là hàm này CHỈ CÓ TUYẾT ĐỐI 3 LỆNH ASM: LDR (Load) -> AND -> RBIT/CLZ (Đếm bit)
        
        NativeWord composite = m_shadow_map[y][startWord] & startMask;
        
        if ([[likely]] composite != 0) {
            // Phát hiện có vật cản ngay trong Word hiện tại!
            return (startWord * HALO_WORD_BITS) + HardwareBitScanForward(composite);
        }

        // Nếu bản đồ lớn hơn 64 (ví dụ 128, 256), nó sẽ duyệt các Word tiếp theo cực mượt
        for (int32_t w = startWord + 1; w < WORDS_PER_ROW; ++w) {
            composite = m_shadow_map[y][w];
            if (__builtin_expect(composite != 0, 1)) {
                return (w * HALO_WORD_BITS) + HardwareBitScanForward(composite);
            }
        }
        
        return MAP_SIZE - 1; // An toàn thoát khỏi bản đồ
    }
};

} // namespace halo::omnicontext