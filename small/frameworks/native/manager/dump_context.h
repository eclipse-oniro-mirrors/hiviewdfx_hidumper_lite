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

#ifndef DUMP_CONTEXT_H
#define DUMP_CONTEXT_H

#include <memory>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>
#include "unique_fd.h"

namespace OHOS {
namespace HiviewDFX {

struct RequestInfo {
    int32_t callingUid = -1;
    int32_t calllingPid = -1;
    UniqueFd outFd;
};

struct DumperOptions {
    int cpuUsagePid = -1;
    int memPid = -1;
    int processPid = -1;
    bool isDumpCpuFreq = false;
    bool isDumpCpuUsage = false;
    bool isDumpMem = false;
    bool isDumpProcesses = false;
};

class DumpContext {
public:
    DumpContext(int32_t uid, int32_t pid, int outFd)
        : requestInfo_(std::make_shared<RequestInfo>(RequestInfo{uid, pid, UniqueFd(outFd)})),
          dumperOpts_(std::make_shared<DumperOptions>()) {}
    ~DumpContext() = default;

    std::shared_ptr<RequestInfo> GetRequestInfo() const { return requestInfo_; }
    std::shared_ptr<DumperOptions> GetDumperOpts() const { return dumperOpts_; }
    int GetOutputFd() const { return requestInfo_->outFd.Get(); }
private:
    std::shared_ptr<RequestInfo> requestInfo_;
    std::shared_ptr<DumperOptions> dumperOpts_;
};

} // namespace HiviewDFX
} // namespace OHOS
#endif // DUMP_CONTEXT_H
