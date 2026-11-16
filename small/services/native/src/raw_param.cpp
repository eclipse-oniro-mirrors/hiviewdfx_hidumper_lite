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
#include "raw_param.h"
#include <cinttypes>
#include <securec.h>
#include <unistd.h>
#include "hilog_wrapper.h"
#include "common/dumper_constant.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
static const bool SHOW_PROGRESS_BAR = false;
static const int PROGRESS_LENGTH = 128;
static const char PROGRESS_STYLE = '=';
static const char PROGRESS_TICK[] = {'-', '\\', '|', '/'};
} // namespace

RawParam::RawParam(int calllingUid, int calllingPid, uint32_t reqId, int argc, char *const argv[], int outfd)
    : uid_(calllingUid), pid_(calllingPid), canceled_(false), finished_(false), reqId_(reqId), outfd_(outfd)
{
    DUMPER_HILOGD(MODULE_SERVICE, "create|pid=%{public}d, reqId=%{public}u", pid_, reqId_);
    isValid_ = Init(argc, argv);
    if (!isValid_) {
        DUMPER_HILOGE(MODULE_SERVICE, "init args failed, argc=%{public}d", argc);
    }
}

RawParam::~RawParam()
{
    DUMPER_HILOGD(MODULE_SERVICE, "release|pid=%{public}d, reqId=%{public}u", pid_, reqId_);
    Uninit();
}

bool RawParam::Init(int argc, char *const argv[])
{
    DUMPER_HILOGD(MODULE_SERVICE, "enter|");
    if (argc < ARG_MIN_COUNT || argc > ARG_MAX_COUNT || argv == nullptr) {
        DUMPER_HILOGE(MODULE_SERVICE, "error|invalid args, argc=%{public}d", argc);
        return false;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i] == nullptr) {
            return false;
        }
        argValues_[i] = std::make_unique<ArgValue>();
        if (sprintf_s(argValues_[i]->value, SINGLE_ARG_MAXLEN, "%s", argv[i]) < 0) {
            return false;
        }
        argHead_[i] = argValues_[i]->value;
    }
    DUMPER_HILOGD(MODULE_SERVICE, "leave|true");
    return true;
}

bool RawParam::IsValid()
{
    return isValid_;
}

void RawParam::Uninit()
{
    DUMPER_HILOGD(MODULE_SERVICE, "enter|");
    CloseOutputFd();
    DUMPER_HILOGD(MODULE_SERVICE, "leave|");
}

int &RawParam::GetOutputFd()
{
    return outfd_;
}

void RawParam::CloseOutputFd()
{
    DUMPER_HILOGD(MODULE_SERVICE, "enter|outfd=%{public}d", outfd_);
    outfd_ = -1;
    DUMPER_HILOGD(MODULE_SERVICE, "leave|");
}

int RawParam::GetArgc()
{
    int count = 0;
    for (int i = 0; i < ARG_MAX_COUNT; i++) {
        if (argHead_[i] == nullptr) {
            break;
        }
        count = i + 1;
    }
    return count;
}

char **RawParam::GetArgv()
{
    return argHead_;
}

int RawParam::GetUid()
{
    return uid_;
}

int RawParam::GetPid()
{
    return pid_;
}

uint32_t RawParam::GetRequestId()
{
    return reqId_;
}

bool RawParam::IsCanceled()
{
    return canceled_;
}

void RawParam::UpdateProgress(uint32_t total, uint32_t current)
{
    if ((!progressEnabled_) || (outfd_ < 0) || (total < 1) || (total < current)) {
        return;
    }
    UpdateProgress((uint64_t(FINISH) * current) / total);
}

void RawParam::UpdateProgress(uint64_t progress)
{
    if ((!progressEnabled_) || (outfd_ < 0) || (progress > FINISH)) {
        return;
    }
    progress_ = std::max(progress, progress_);
    progressTick_ = (progressTick_ + 1) % sizeof(PROGRESS_TICK);
    if (SHOW_PROGRESS_BAR) {
        char barbuf[PROGRESS_LENGTH + 1] = {0};
        for (size_t i = 0; ((i < progress_) && (i < PROGRESS_LENGTH)); i++) {
            barbuf[i] = PROGRESS_STYLE;
        }
        dprintf(outfd_, "\033[?25l\r[%-100s],%2" PRIu64 "%%,[%c]\033[?25h",
            barbuf, progress_, PROGRESS_TICK[progressTick_]);
    } else {
        dprintf(outfd_, "\033[?25l\r%2" PRIu64 "%%,[%c]\033[?25h", progress_, PROGRESS_TICK[progressTick_]);
    }
}
} // namespace HiviewDFX
} // namespace OHOS
