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

#include "executor/memory/get_heap_info.h"
#include "executor/memory/memory_util.h"
#include "executor/memory/memory_info.h"
#include "hilog_wrapper.h"
#include "util/string_utils.h"
using namespace std;
namespace OHOS {
namespace HiviewDFX {
GetHeapInfo::GetHeapInfo()
{
}

GetHeapInfo::~GetHeapInfo()
{
}
void GetHeapInfo::GetMallocHeapInfo(const int& pid, std::unique_ptr<MallHeapInfo>& mallocHeapInfo)
{
    DUMPER_HILOGI(MODULE_SERVICE, "GetMallocHeapInfo pid:%{public}d begin.", pid);
    mallocHeapInfo->size = 0;
    mallocHeapInfo->alloc = 0;
    mallocHeapInfo->free = 0;
    DUMPER_HILOGI(MODULE_SERVICE, "GetMallocHeapInfo pid:%{public}d end, success!", pid);
}
} // namespace HiviewDFX
} // namespace OHOS
