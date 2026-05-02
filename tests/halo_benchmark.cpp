#include <benchmark/benchmark.h>
#include "/Users/nguyenmanhhung/halo-aegis-core/include/halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

static void BM_Halo_EscapeRaycast(benchmark::State& state) {
    AdaptiveOmniEngine aegis;
    aegis.Init();

    // Thiết lập vật cản mẫu (15 layers)
    for(int i = 0; i < 15; ++i) aegis.SetBit(i, 30, 15);

    // Vòng lặp đo lường của Google Benchmark
    for (auto _ : state) {
        // Trình biên dịch sẽ không thể optimize-out nhờ cơ chế nội bộ của Benchmark
        benchmark::DoNotOptimize(aegis.EscapeRaycast(2, 15));
    }
    
    // Thêm các chỉ số phụ để bồ dễ theo dõi
    state.SetItemsProcessed(state.iterations());
}

// Đăng ký hàm benchmark
BENCHMARK(BM_Halo_EscapeRaycast);

// Chạy benchmark
BENCHMARK_MAIN();