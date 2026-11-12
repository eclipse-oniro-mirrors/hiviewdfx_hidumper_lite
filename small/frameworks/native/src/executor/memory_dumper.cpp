/*
 * Copyright (C) 2021-2022 Huawei Device Co., Ltd.
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
#include "executor/memory_dumper.h"
#include "executor/memory/memory_info_wrapper.h"
#include "dump_utils.h"
#include "common/dumper_constant.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {

MemoryDumper::MemoryDumper()
{
}

MemoryDumper::~MemoryDumper()
{
}

DumpStatus MemoryDumper::PreExecute(const shared_ptr<DumperParameter> &parameter, StringMatrix dumpDatas)
{
    pid_ = parameter->GetOpts().memPid_;
    timeInterval_ = parameter->GetOpts().timeInterval_;
    isReceivedSigInt_ = parameter->GetOpts().isReceivedSigInt_;
    dumpMemPrune_ = parameter->GetOpts().dumpMemPrune_;
    dumpDatas_ = dumpDatas;

    auto callback = parameter->getClientCallback();
    if (callback == nullptr) {
        DUMPER_HILOGE(MODULE_SERVICE, "PreExecute error|callback is nullptr");
        return DumpStatus::DUMP_FAIL;
    }
    rawParamFd_ = callback->GetOutputFd();
    return DumpStatus::DUMP_OK;
}

DumpStatus MemoryDumper::Execute()
{
    DUMPER_HILOGI(MODULE_SERVICE, "info|MemoryDumper Execute enter");
    if (isReceivedSigInt_) {
        SetReceivedSigInt();
        DUMPER_HILOGI(MODULE_SERVICE, "ctrl+c, pid:%{public}d", pid_);
        return status_;
    }
    if (dumpDatas_ != nullptr) {
        if (pid_ >= 0) {
            if (timeInterval_ > 0) {
                GetMemByTimeInterval();
                DUMPER_HILOGI(MODULE_SERVICE, "GetMemByTimeInterval end,pid:%{public}d", pid_);
                return status_;
            }
            GetMemByPid();
        } else {
            if (dumpMemPrune_) {
                GetMemPruneNoPid();
            } else {
                GetMemNoPid();
            }
        }
    }
    DUMPER_HILOGI(MODULE_SERVICE, "info|MemoryDumper Execute end");
    return status_;
}

void MemoryDumper::GetMemByPid()
{
    int ret = GetMemoryInfoByPid(pid_, dumpDatas_);
    if (ret == 0) {
        status_ = DumpStatus::DUMP_OK;
    } else {
        DUMPER_HILOGE(MODULE_SERVICE, "MemoryDumper Execute failed, pid:%{public}d", pid_);
        status_ = DumpStatus::DUMP_FAIL;
    }
}

void MemoryDumper::GetMemNoPid()
{
    status_ = static_cast<DumpStatus>(GetMemoryInfoNoPid(rawParamFd_, dumpDatas_));
}

void MemoryDumper::GetMemPruneNoPid()
{
    status_ = static_cast<DumpStatus>(GetMemoryInfoPrune(rawParamFd_, dumpDatas_));
}

void MemoryDumper::GetMemByTimeInterval()
{
    GetMemoryInfoByTimeInterval(rawParamFd_, pid_, timeInterval_);
    status_ = DumpStatus::DUMP_OK;
}

void MemoryDumper::SetReceivedSigInt()
{
    ::SetReceivedSigInt(isReceivedSigInt_);
    status_ = DumpStatus::DUMP_OK;
}

DumpStatus MemoryDumper::AfterExecute()
{
    return status_;
}
} // namespace HiviewDFX
} // namespace OHOS
