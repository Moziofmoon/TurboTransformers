// Copyright (C) 2020 THL A29 Limited, a Tencent company.
// All rights reserved.
// Licensed under the BSD 3-Clause License (the "License"); you may
// not use this file except in compliance with the License. You may
// obtain a copy of the License at
// https://opensource.org/licenses/BSD-3-Clause
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" basis,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
// implied. See the License for the specific language governing
// permissions and limitations under the License.
// See the AUTHORS file for names of contributors.

#include "benchmark_help.h"
#include "turbo_transformers/layers/kernels/transpose.h"

#include <chrono>

#include "catch2/catch.hpp"
#include "loguru.hpp"
#include "turbo_transformers/core/tensor.h"
#include "turbo_transformers/layers/kernels/common.h"

namespace turbo_transformers {
namespace layers {
namespace kernels {

static void SplitAddTransposeBenchmarkHelper(int batch_size, int seq_length,
                                             int num_attention_heads,
                                             const std::string& info,
                                             DLDeviceType dev, int n_step) {
  auto g_bytes = batch_size * num_attention_heads * 3 * seq_length * 64 *
                 sizeof(float) / 1e9;
  core::Tensor input_tensor(core::NewDLPackTensorT<float>(
      {batch_size, seq_length, 3, num_attention_heads, 64}, dev, 0));
  common::FillRandom<float>(input_tensor);
  core::Tensor bias_tensor(
      core::NewDLPackTensorT<float>({3, num_attention_heads, 64}, dev, 0));
  common::FillRandom<float>(bias_tensor);
  turbo_transformers::core::Tensor output_tensor(
      turbo_transformers::core::NewDLPackTensorT<float>(
          {3, batch_size, num_attention_heads, seq_length, 64}, dev, 0));

  auto res = benchmark::TestFuncSpeed(
      [&]() {
        SplitAddBiasTransposeForScore(&output_tensor, input_tensor,
                                      bias_tensor);
      },
      n_step, info, g_bytes, dev);

  std::cout << "GPU SplitAddTranspose " << batch_size << ", " << seq_length
            << " , " << res << " GB/s" << std::endl;
}

TEST_CASE("transpose-cpu-benchmark") {
  constexpr int64_t num_attention_heads = 12;
  constexpr int n_step = 150;

  std::vector<int64_t> batch_size_list{1, 20};
  std::vector<int64_t> seq_length_list{10,  20,  40,  60,  80,
                                       100, 200, 300, 400, 500};

  for (auto batch_size : batch_size_list)
    for (auto seq_length : seq_length_list) {
      std::stringstream ss;
      SplitAddTransposeBenchmarkHelper(batch_size, seq_length,
                                       num_attention_heads, ss.str(), kDLCPU,
                                       n_step);
    }
}

#ifdef TT_WITH_CUDA
TEST_CASE("softmax-gpu-benchmark") {
  constexpr int64_t num_attention_heads = 12;

  constexpr int n_step = 150;

  std::vector<int64_t> batch_size_list{1, 20};
  std::vector<int64_t> seq_length_list{10,  20,  40,  60,  80,
                                       100, 200, 300, 400, 500};

  for (auto batch_size : batch_size_list)
    for (auto seq_length : seq_length_list) {
      std::stringstream ss;
      SplitAddTransposeBenchmarkHelper(batch_size, seq_length,
                                       num_attention_heads, ss.str(), kDLGPU,
                                       n_step);
    }
}
#endif

}  // namespace kernels
}  // namespace layers
}  // namespace turbo_transformers
