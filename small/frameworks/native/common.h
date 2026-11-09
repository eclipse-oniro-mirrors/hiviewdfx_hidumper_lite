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
#ifndef HIDUMPER_COMMON_H
#define HIDUMPER_COMMON_H
namespace OHOS {
namespace HiviewDFX {
enum DumpStatus {
    DUMP_REQUEST_MAX = -5,
    DUMP_TIMEOUT = -4,
    DUMP_INVALID_ARG = -3,
    DUMP_NOPERMISSION = -2,
    DUMP_FAIL = -1,
    DUMP_OK = 0,
    DUMP_HELP,
    DUMP_CHECK_OK,
    DUMP_MORE_DATA,
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
