/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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
#ifndef HIDUMPER_SERVICES_DUMPER_CONSTANT_H
#define HIDUMPER_SERVICES_DUMPER_CONSTANT_H

#include <cstdint>
#include <string>
#include <vector>
#include <iostream>
#include <memory>

namespace OHOS {
namespace HiviewDFX {
enum DumperConstant {
    NONE,
    GROUP,        // group
    DUMPER_BEGIN, // dumper begin
    CPU_DUMPER,
    FILE_DUMPER,
    CMD_DUMPER,
    MEMORY_DUMPER,
    DUMPER_END,   // dumper end
    FILTER_BEGIN, // filter begin
    FILTER_END,   // filter end
    OUTPUT_BEGIN, // output begin
    FD_OUTPUT,
    OUTPUT_END,  // output end
    LEVEL_BEGIN, // level begin
    LEVEL_NONE,
    LEVEL_MIDDLE,
    LEVEL_HIGH,
    LEVEL_ALL,
    LEVEL_END,       // level end
    GROUPTYPE_BEGIN, // group type begin
    GROUPTYPE_PID,   // pid of group type
    GROUPTYPE_CPUID, // cpuid of group type
    GROUPTYPE_END,   // group type end
    LOOP,
};

const uint64_t FDTAG = (((uint64_t)0xD002D20 << 32) | 0x01);
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIDUMPER_SERVICES_DUMPER_CONSTANT_H
