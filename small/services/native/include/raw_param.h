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
#ifndef HIDUMPER_SERVICES_DUMP_RAW_PARAM_H
#define HIDUMPER_SERVICES_DUMP_RAW_PARAM_H
#include <memory>
#include <mutex>
#include <string_ex.h>
#include <vector>
#include "dump_controller.h"
namespace OHOS {
namespace HiviewDFX {
class RawParam {
public:
    RawParam(int calllingUid, int calllingPid, uint32_t reqId, int argc, char *const argv[], int outfd);
    ~RawParam();
    int GetArgc();
    char** GetArgv();
    int GetUid();
    int GetPid();
    uint32_t GetRequestId();
    bool IsCanceled();
public:
    void UpdateProgress(uint32_t total, uint32_t current);
    void UpdateProgress(uint64_t progress);
    int& GetOutputFd();
    void CloseOutputFd();
    bool Init(int argc, char *const argv[]);
    void Uninit();
private:
    struct ArgValue {
        char value[SINGLE_ARG_MAXLEN + 1] = {0};
    };
    std::mutex mutex_;
    int uid_ {-1};
    int pid_ {-1};
    bool canceled_ {false};
    bool finished_ {false};
    bool hasError_ {false};
    uint32_t reqId_ {0};
    char* argHead_[ARG_MAX_COUNT + 1] = {nullptr};
    std::unique_ptr<ArgValue> argValues_[ARG_MAX_COUNT + 1] = {nullptr};
    int outfd_ {-1};
    bool progressEnabled_ {false};
    uint32_t progressTick_ {0};
    uint64_t progress_ {0};
    const uint64_t FINISH = 100;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif // HIDUMPER_SERVICES_DUMP_RAW_PARAM_H
