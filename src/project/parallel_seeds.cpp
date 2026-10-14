#include "parallel_seeds.hpp"
#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <thread>

namespace trafficsim {
std::vector<SeedRun> runSeedsParallel(const Scenario& scenario, const EvaluationSpec& spec,
                                      const std::vector<std::uint32_t>& seeds,
                                      const std::function<bool(std::size_t)>& progress, unsigned threads,
                                      const std::function<bool()>& cancelled) {
    if (threads == 0) threads = std::max(1u, std::thread::hardware_concurrency());
    if (threads <= 1 || seeds.size() <= 1) return runSeeds(scenario, spec, seeds, progress, cancelled);
    {   // runSeeds' own refusal, before any thread starts.
        auto sorted = seeds;
        std::sort(sorted.begin(), sorted.end());
        if (std::adjacent_find(sorted.begin(), sorted.end()) != sorted.end())
            throw std::invalid_argument("duplicate seed in batch");
    }
    // Workers claim the next index and write its slot. Declared before `workers`, so all of this
    // outlives every join, including when `progress` throws.
    std::vector<SeedRun> runs(seeds.size());
    std::mutex mutex;
    std::condition_variable finished;
    std::size_t done = 0;
    std::exception_ptr failure;
    std::atomic<std::size_t> next{0};
    std::atomic<bool> stop{false};
    std::vector<std::jthread> workers;
    const auto count = std::min<std::size_t>(threads, seeds.size());
    try { for (std::size_t t = 0; t < count; ++t)
        workers.emplace_back([&] {
            for (std::size_t i; !stop && (i = next++) < seeds.size();) {
                try {
                    runs[i] = runSeed(scenario, spec, seeds[i], 0.05, [&] { return stop || (cancelled && cancelled()); });
                } catch (const RunCancelled&) {
                    const std::lock_guard lock(mutex);
                    stop = true; finished.notify_one(); return;
                } catch (...) {
                    const std::lock_guard lock(mutex);
                    if (!failure) failure = std::current_exception();
                    stop = true;
                    finished.notify_one();
                    return;
                }
                const std::lock_guard lock(mutex);
                ++done;
                finished.notify_one();
            }
        });
    } catch (...) { stop = true; throw; } // thread creation failure must stop existing seeds before joining
    bool aborted = false;
    try {
        std::unique_lock lock(mutex);
        for (std::size_t reported = 0; reported < seeds.size() && !aborted;) {
            finished.wait(lock, [&] { return done > reported || failure != nullptr || stop; });
            if (failure) break;
            if (stop) { aborted = true; break; }
            for (const auto now = done; reported < now && !aborted;) {
                ++reported;
                lock.unlock();
                aborted = progress && !progress(reported);
                lock.lock();
            }
        }
    } catch (...) {
        stop = true;
        throw;
    }
    stop = true;
    workers.clear(); // joins
    if (failure) std::rethrow_exception(failure);
    if (aborted || (cancelled && cancelled())) return {};
    return runs;
}
}
