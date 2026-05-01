/**
 * ============================================================================
 *  H.A.L.O.* AURA KERNEL — Multi-Threading Task Dispatcher
 * ============================================================================
 */
#pragma once
#include "halo_core.h"
#include <vector>
#include <future>
#include <thread>

namespace halo::kernel {

class AuraDispatcher {
    std::vector<HaloPathfinder> m_workers;
    std::vector<memory::ArenaAllocator> m_arenas;
    int32_t m_threadCount;

public:
    void Init(Grid* sharedGrid, int32_t threads) {
        m_threadCount = threads;
        m_workers.resize(threads);
        m_arenas.resize(threads);
        
        // Cấp cho mỗi Luồng (Thread) một Arena 2MB độc lập để tụi nó không đánh lộn nhau giành RAM
        for (int i = 0; i < threads; ++i) {
            m_arenas[i].Init(2 * 1024 * 1024);
            m_workers[i].Init(sharedGrid, m_arenas[i]);
        }
    }

    // Xử lý cả ngàn lộ trình cùng lúc!
    std::vector<PathResult> DispatchBatchRoutes(const std::vector<std::pair<Vec2i, Vec2i>>& requests) {
        std::vector<PathResult> finalResults(requests.size());
        std::vector<std::future<void>> futures;

        // Chia đều đống request cho các nhân CPU
        int32_t chunkSize = (requests.size() + m_threadCount - 1) / m_threadCount;

        for (int t = 0; t < m_threadCount; ++t) {
            int32_t startIdx = t * chunkSize;
            int32_t endIdx = std::min(startIdx + chunkSize, static_cast<int32_t>(requests.size()));

            if (startIdx >= requests.size()) break;

            // Đẩy việc qua thread khác chạy ngầm (Asynchronous)
            futures.push_back(std::async(std::launch::async, [this, t, startIdx, endIdx, &requests, &finalResults]() {
                for (int i = startIdx; i < endIdx; ++i) {
                    finalResults[i] = m_workers[t].FindPath(requests[i].first, requests[i].second);
                }
            }));
        }

        // Chờ tất cả các nhân CPU báo cáo kết quả về
        for (auto& f : futures) {
            f.get();
        }

        return finalResults;
    }
};
} // namespace halo::kernel