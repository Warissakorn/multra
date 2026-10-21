#pragma once
#include "../runner/batch.hpp"

namespace multra {
// D152: runSeeds with up to `threads` seeds at once (0: one per hardware thread; 1: runSeeds itself,
// inline). The runner stays thread-free (check_architecture); this only schedules its runSeed. Each
// seed is still one single-threaded run written to its own slot, so the result is identical to
// runSeeds whatever `threads` is: the same runs in the requested order, duplicates refused.
// `progress` is called on the calling thread only, once per finished seed, in increasing order;
// returning false stops the workers at a tick checkpoint and returns nothing. A seed that
// throws stops the rest, and its exception reaches the caller after every worker has joined.
std::vector<SeedRun> runSeedsParallel(const Scenario&, const EvaluationSpec&, const std::vector<std::uint32_t>& seeds,
                                      const std::function<bool(std::size_t done)>& progress = {}, unsigned threads = 0,
                                      const std::function<bool()>& cancelled = {}); // called by workers; must be thread-safe
}
