#include <benchmark/benchmark.h>
#include "/Users/nguyenmanhhung/halo-aegis-core/include/halo/core/halo_omnicontext_core.h"

using namespace halo::omnicontext;

static void BM_Halo_EscapeRaycast(benchmark::State& state) {
    AdaptiveOmniEngine aegis;
    aegis.Init();

    for(int i = 0; i < 15; ++i) aegis.SetBit(i, 30, 15);

    for (auto _ : state) {
        benchmark::DoNotOptimize(aegis.EscapeRaycast(2, 15));
    }
    
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_Halo_EscapeRaycast);

BENCHMARK_MAIN();