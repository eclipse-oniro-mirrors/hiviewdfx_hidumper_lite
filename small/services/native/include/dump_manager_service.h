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
#ifndef HIDUMPER_SERVICES_DUMP_MANAGER_SERVICE_H
#define HIDUMPER_SERVICES_DUMP_MANAGER_SERVICE_H
#include <memory>
#include "singleton.h"
#include "raw_param.h"

namespace OHOS {
namespace HiviewDFX {

class DumpManagerService final : public DelayedRefSingleton<DumpManagerService> {
    DECLARE_DELAYED_REF_SINGLETON(DumpManagerService)
    DISALLOW_COPY_AND_MOVE(DumpManagerService);
public:
    // Used for dump request, called by the hidumper command line entry directly.
    int32_t Request(int argc, char *argv[], int outfd);
private:
    int32_t RequestMain(const std::shared_ptr<RawParam>& rawParam);
    uint32_t GetRequestId();
private:
    uint32_t requestIndex_ {0};
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIDUMPER_SERVICES_DUMP_MANAGER_SERVICE_H
