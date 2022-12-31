/** @file mem_pool.cpp
*/
#if not DISABLE_MEM_POOL
#include <cuda.h>
#include "common.h"
#include "mem_pool.h"

using namespace Impl;
using namespace std;

static std::unordered_map<uint64_t, std::deque<void *>> mem_cache;
static std::unordered_map<void *, uint64_t> mem_size;

static constexpr size_t cache_memory_step = 512 * 1024;

namespace Impl {

void init_mem_pool() {}

void deinit_mem_pool() {
#if DEBUG
  vector<pair<uint64_t, int>> sizes;
  for (auto &kv : mem_cache) {
    sizes.push_back({kv.first, kv.second.size()});
  }
  sort(sizes.begin(), sizes.end(), [](const pair<uint64_t, int> &a, const pair<uint64_t, int> &b) {
    return a.first < b.first;
  });
  auto total = 0;
  for (auto &kv : sizes) {
    cout << "mem_pool: " << kv.first / cache_memory_step << "*512KB" << ": " << kv.second << endl;
    total += kv.first * kv.second;
  }

  total /= 1024 * 1024;

  cout << "mem_pool: total " << total << "MB" << endl;
#endif

  for (auto &kv : mem_cache)
    for (auto &ptr : kv.second)
      checkCudaErrors(cudaFree(ptr));
}

cudaError_t cudaPooledMalloc(void **devPtr, size_t size) {
  auto stepped_size = (size + cache_memory_step - 1) / cache_memory_step * cache_memory_step;
  if (mem_cache.count(stepped_size) > 0 && !mem_cache[stepped_size].empty()) {
    *devPtr = mem_cache[stepped_size].front();
    mem_cache[stepped_size].pop_front();
    return cudaSuccess;
  } else {
    auto ret = cudaMalloc(devPtr, stepped_size);
    if (ret == cudaSuccess) {
      mem_size[*devPtr] = stepped_size;
    }
    return ret;
  }
}

cudaError_t cudaPooledFree(void *devPtr) {
  auto size = mem_size[devPtr];
  mem_cache[size].push_back(devPtr);
  return cudaSuccess;
}

}
#endif

