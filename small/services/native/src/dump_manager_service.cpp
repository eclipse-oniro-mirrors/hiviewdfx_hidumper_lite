/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "dump_manager_service.h"
#include <unistd.h>
#include "common.h"
#include "hilog_wrapper.h"
#include "manager/dump_implement.h"
#include "raw_param.h"

using namespace std;
namespace OHOS {
namespace HiviewDFX {
namespace {
static const uint32_t REQUESTID_MAX = 100000;
}

DumpManagerService::DumpManagerService()
{
}

DumpManagerService::~DumpManagerService()
{
}

uint32_t DumpManagerService::GetRequestId()
{
    requestIndex_ = (requestIndex_ + 1) % REQUESTID_MAX;
    return requestIndex_;
}

int32_t DumpManagerService::Request(int argc, char *argv[], int outfd)
{
    DUMPER_HILOGD(MODULE_SERVICE, "enter|");
    auto rawParam = std::make_shared<RawParam>(getuid(), getpid(), GetRequestId(), argc, argv, outfd);
    int32_t ret = RequestMain(rawParam);
    DUMPER_HILOGD(MODULE_SERVICE, "leave|ret=%{public}d", ret);
    return ret;
}

int32_t DumpManagerService::RequestMain(const std::shared_ptr<RawParam>& rawParam)
{
    DUMPER_HILOGD(MODULE_SERVICE, "enter|");
    int32_t ret = DumpStatus::DUMP_FAIL;
    int argC = rawParam->GetArgc();
    char **argV = rawParam->GetArgv();
    if ((argC > 0) && (argV != nullptr)) {
        DUMPER_HILOGD(MODULE_SERVICE, "debug|enter task, argC=%{public}d", argC);
        ret = DumpImplement::GetInstance().Main(argC, argV, rawParam);
        DUMPER_HILOGD(MODULE_SERVICE, "debug|leave task, ret=%{public}d", ret);
    }
    rawParam->CloseOutputFd();
    DUMPER_HILOGD(MODULE_SERVICE, "leave|");
    return ret;
}
} // namespace HiviewDFX
} // namespace OHOS
