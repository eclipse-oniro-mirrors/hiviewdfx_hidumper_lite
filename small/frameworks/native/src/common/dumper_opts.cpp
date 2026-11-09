/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "common/dumper_opts.h"

#include <string>

#include "hilog_wrapper.h"

namespace OHOS {
namespace HiviewDFX {

DumperOpts::DumperOpts()
{
    Reset();
}

void DumperOpts::Reset()
{
    ResetCpuOptions();
    ResetMemOptions();
    ResetProcessOptions();
}

void DumperOpts::ResetCpuOptions()
{
    isDumpCpuFreq_ = false;
    isDumpCpuUsage_ = false;
    cpuUsagePid_ = -1;
}

void DumperOpts::ResetMemOptions()
{
    isDumpMem_ = false;
    isReceivedSigInt_ = false;
    dumpMemPrune_ = false;
    timeInterval_ = 0;
    memPid_ = -1;
}

void DumperOpts::ResetProcessOptions()
{
    isDumpProcesses_ = false;
    processPid_ = -1;
}

DumperOpts& DumperOpts::operator=(const DumperOpts& opts)
{
    Reset();

    AssignCpuOptions(opts);
    AssignMemOptions(opts);
    AssignProcessOptions(opts);

    return *this;
}

void DumperOpts::AssignCpuOptions(const DumperOpts& opts)
{
    isDumpCpuFreq_ = opts.isDumpCpuFreq_;
    isDumpCpuUsage_ = opts.isDumpCpuUsage_;
    cpuUsagePid_ = opts.cpuUsagePid_;
}

void DumperOpts::AssignMemOptions(const DumperOpts& opts)
{
    isDumpMem_ = opts.isDumpMem_;
    isReceivedSigInt_ = opts.isReceivedSigInt_;
    timeInterval_ = opts.timeInterval_;
    memPid_ = opts.memPid_;
    dumpMemPrune_ = opts.dumpMemPrune_;
}

void DumperOpts::AssignProcessOptions(const DumperOpts& opts)
{
    isDumpProcesses_ = opts.isDumpProcesses_;
    processPid_ = opts.processPid_;
}

bool DumperOpts::IsSelectAny() const
{
    if (isDumpCpuFreq_ || isDumpCpuUsage_) {
        return true;
    }
    if (isDumpMem_) {
        return true;
    }
    if (isDumpProcesses_) {
        return true;
    }
    DUMPER_HILOGE(MODULE_COMMON, "select nothing.");
    return false;
}

bool DumperOpts::CheckOptions(std::string& errStr) const
{
    if (cpuUsagePid_ < -1) {
        errStr = std::to_string(cpuUsagePid_);
        return false;
    }
    if (memPid_ < -1) {
        errStr = std::to_string(memPid_);
        return false;
    }
    if (processPid_ < -1) {
        errStr = std::to_string(processPid_);
        return false;
    }
    if (timeInterval_ < 0) {
        errStr = std::to_string(timeInterval_);
        return false;
    }
    return true;
}
} // namespace HiviewDFX
} // namespace OHOS
