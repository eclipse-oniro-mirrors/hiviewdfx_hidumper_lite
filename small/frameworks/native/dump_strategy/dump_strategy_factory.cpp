/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "dump_strategy_factory.h"
#include "hilog_wrapper.h"
#include "cpu_dump_strategy.h"
#include "mem_dump_strategy.h"
#include "process_info_dump_strategy.h"

namespace OHOS {
namespace HiviewDFX {

std::vector<StrategyConfig> DumpStrategyFactory::strategyConfigs_ = {
    {
        [](const std::shared_ptr<DumperOptions>& opts) {
            return opts->isDumpCpuFreq || opts->isDumpCpuUsage;
        },
        []() { return std::make_unique<CpuDumpStrategy>(); }
    },
    {
        [](const std::shared_ptr<DumperOptions>& opts) { return opts->isDumpMem; },
        []() { return std::make_unique<MemDumpStrategy>(); }
    },
    {
        [](const std::shared_ptr<DumperOptions>& opts) { return opts->isDumpProcesses; },
        []() { return std::make_unique<ProcessInfoDumpStrategy>(); }
    }
};

DumpStrategyFactory::DumpStrategyFactory() = default;
DumpStrategyFactory::~DumpStrategyFactory() = default;

void DumpStrategyFactory::CreateStrategies(const std::shared_ptr<DumperOptions>& opts,
                                           std::vector<std::unique_ptr<DumpStrategy>>& strategies)
{
    for (const auto& config : strategyConfigs_) {
        if (config.condition(opts)) {
            auto strategy = config.creator();
            if (strategy) {
                strategies.emplace_back(std::move(strategy));
            }
        }
    }
}

DumpStatus DumpStrategyFactory::ExecuteStrategies(const std::vector<std::unique_ptr<DumpStrategy>>& strategies,
                                                  const DumpContext& context, std::vector<TaskId>& rootTasks)
{
    if (strategies.empty()) {
        DUMPER_HILOGE(MODULE_COMMON, "Strategy execution failed");
        return DumpStatus::DUMP_FAIL;
    }
    DumpStatus ret = DumpStatus::DUMP_OK;
    for (const auto& strategy : strategies) {
        ret = strategy->CollectRootTasks(context, rootTasks);
        if (ret != DumpStatus::DUMP_OK) {
            DUMPER_HILOGE(MODULE_COMMON, "Strategy failed");
            break;
        }
    }
    return ret;
}

DumpStatus DumpStrategyFactory::CollectRootTasks(const DumpContext& context, std::vector<TaskId>& rootTasks)
{
    auto opts = context.GetDumperOpts();
    if (opts == nullptr) {
        DUMPER_HILOGE(MODULE_COMMON, "Failed to get options from context");
        return DumpStatus::DUMP_FAIL;
    }

    std::vector<std::unique_ptr<DumpStrategy>> strategies;
    CreateStrategies(opts, strategies);
    return ExecuteStrategies(strategies, context, rootTasks);
}

} // namespace HiviewDFX
} // namespace OHOS
