#include "Future.h"
#include <benchmark/benchmark.h>
#include <cassert>

static future<int> ring(future<int> f) {
  int val = co_await f;
  co_return val + 1;
}

static future<int> BM_RingActor(benchmark::State *benchState) {

  while (benchState->KeepRunning()) {
    benchState->PauseTiming();

    std::vector<future<int>> futures;
    futures.reserve(benchState->range(0));

    promise<int> p;
    futures.push_back(ring(p.get_future()));
    for (int i = 1; i < benchState->range(0); ++i) {
      futures.push_back(ring(futures.back()));
    }

    benchState->ResumeTiming();

    p.set_value(0);
    int a = co_await(futures.back());

    assert(a == benchState->range(0));
  }

  benchState->SetItemsProcessed(static_cast<long>(benchState->iterations()) *
                                benchState->range(0));

  co_return 1;
}

static void bench_ring_coroutine(benchmark::State &benchState) {
  BM_RingActor(&benchState);
}

BENCHMARK(bench_ring_coroutine)->RangeMultiplier(2)->Range(8 << 2, 8 << 11);

BENCHMARK_MAIN();
