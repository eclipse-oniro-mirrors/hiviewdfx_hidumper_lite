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
#include "dump_utils.h"

#include <cerrno>
#include <csignal>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <climits>

#include <sys/prctl.h>
#include <sys/resource.h>

#include "common.h"
#include "common/dumper_constant.h"
#include "file_ex.h"
#include "securec.h"
#include "string_ex.h"

#include "hilog_wrapper.h"

namespace OHOS {
namespace HiviewDFX {
void DumpUtils::IgnorePipeQuit()
{
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        DUMPER_HILOGE(MODULE_COMMON, "signal SIGPIPE IGN failed, errno=%{public}d", errno);
    }
}

void DumpUtils::IgnoreStdoutCache()
{
    setvbuf(stdout, nullptr, _IONBF, 0); // never cache stdout
}

void DumpUtils::IgnoreOom()
{
    setpriority(PRIO_PROCESS, 0, topPriority); // protect from OOM
}

void DumpUtils::SetAdj(int adj)
{
    int fd = FdToWrite(fileCurOomAdj);
    if (fd < 0) {
        return;
    }
    fdsan_exchange_owner_tag(fd, 0, FDTAG);
    dprintf(fd, "%d", adj);
    fdsan_close_with_tag(fd, FDTAG);
}

void DumpUtils::BoostPriority()
{
    IgnoreOom();
    IgnorePipeQuit();
    IgnoreStdoutCache();
    SetAdj(topOomAdj);
}

std::string DumpUtils::ErrnoToMsg(const int &error)
{
    const int bufSize = 128;
    char buf[bufSize] = {0};
    strerror_r(error, buf, bufSize);
    return buf;
}

int DumpUtils::FdToRead(const std::string &file)
{
    char path[PATH_MAX] = {0};
    if (realpath(file.c_str(), path) == nullptr) {
        DUMPER_HILOGE(MODULE_COMMON, "realpath, no such file. path=[%{public}s], file=[%{public}s]",
            path, file.c_str());
        return -1;
    }

    if (file != std::string(path)) {
        DUMPER_HILOGI(MODULE_COMMON, "fail to check consistency. path=[%{public}s], file=[%{public}s]",
            path, file.c_str());
    }

    int fd = TEMP_FAILURE_RETRY(open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK));
    if (fd == -1) {
        DUMPER_HILOGE(MODULE_COMMON, "open [%{public}s] %{public}s", path, ErrnoToMsg(errno).c_str());
    }
    return fd;
}

int DumpUtils::FdToWrite(const std::string &file)
{
    std::string split = "/";
    auto pos = file.find_last_of(split);
    if (pos == std::string::npos) {
        DUMPER_HILOGE(MODULE_COMMON, "file path:[%{public}s] error", file.c_str());
        return -1;
    }
    std::string tempPath = file.substr(0, pos + 1);
    std::string name = file.substr(pos + 1);

    char path[PATH_MAX] = {0};
    if (realpath(tempPath.c_str(), path) != nullptr) {
        std::string fileName = path + split + name;
        int fd = -1;
        if (access(fileName.c_str(), F_OK) == 0) {
            fd = open(fileName.c_str(), O_WRONLY);
            if (lseek(fd, 0, SEEK_END) == -1) {
                DUMPER_HILOGE(MODULE_COMMON, "lseek fail fd:%{public}d, errno:%{public}d", fd, errno);
            }
        } else {
            fd = TEMP_FAILURE_RETRY(open(fileName.c_str(), O_WRONLY | O_CREAT | O_CLOEXEC | O_NOFOLLOW,
                                         S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH));
        }
        if (fd == -1) {
            DUMPER_HILOGE(MODULE_COMMON, "open [%{public}s] %{public}s",
                fileName.c_str(), ErrnoToMsg(errno).c_str());
        }
        return fd;
    }
    DUMPER_HILOGD(MODULE_COMMON, "realpath, no such file. path=[%{public}s], tempPath=[%{public}s]",
        path, tempPath.c_str());
    return -1;
}

bool DumpUtils::CheckProcessAlive(uint32_t pid)
{
    char path[PATH_MAX] = {0};
    char pathPid[PATH_MAX] = {0};
    int ret = sprintf_s(pathPid, PATH_MAX, "/proc/%u", pid);
    if (ret < 0) {
        return false;
    }
    if (realpath(pathPid, path) != nullptr) { // path exist
        if (access(path, F_OK) == 0) {        // can be read
            return true;
        }
    }
    return false;
}

bool DumpUtils::PathIsValid(const std::string &path)
{
    return access(path.c_str(), F_OK) == 0;
}
} // namespace HiviewDFX
} // namespace OHOS
