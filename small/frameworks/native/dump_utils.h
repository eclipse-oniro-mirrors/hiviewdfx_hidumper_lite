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
#ifndef DUMP_UTILS_H
#define DUMP_UTILS_H
#include <string>
#include <vector>
#include <unistd.h>

#ifdef HIDUMPER_BUNDLEMANAGER_FRAMEWORK_ENABLE
#include "application_info.h"
#endif

namespace OHOS {
namespace HiviewDFX {
#ifndef TEMP_FAILURE_RETRY
#define TEMP_FAILURE_RETRY
#endif
class DumpUtils {
public:
    // system operations
    static void IgnoreOom();
    static void IgnorePipeQuit();
    static void IgnoreStdoutCache();
    static void SetAdj(int adj);
    static void BoostPriority();

    // string operations
    static void RemoveDuplicateString(std::vector<std::string> &vector);

    // file operations
    static int FdToRead(const std::string &file);
    static int FdToWrite(const std::string &file);

    // process operations
    static bool CheckProcessAlive(uint32_t pid);

    // ability string to ID
    static int StrToId(const std::string &name); // -1:invalid
    static std::string ConvertSaIdToSaName(const std::string &saIdStr);

    static bool DirectoryExists(const std::string &path);
    static bool PathIsValid(const std::string &path);
    static bool CopyFile(const std::string &src, const std::string &des);
    static bool IsCommercialVersion();
    static bool IsUserMode();
    static bool CheckAppDebugVersion(int pid);
    static bool IsHmKernel();
    static bool CheckAppProfileable(int pid);
    static bool CheckAppInhouse(int pid);
    static bool IsDeveloperModeEnabled();
    static constexpr int TOP_PRIORITY = -20;
    static constexpr int TOP_OOM_ADJ = -1000;
    static constexpr char FILE_CUR_OOM_ADJ[] = "/proc/self/oom_score_adj";
    static constexpr auto& SPACE = " ";
    static const int INVALID_PID = -1;
private:
    static bool GetBundleNameByPid(int pid, std::string &bundleName);
    static std::string ErrnoToMsg(const int &error);
#ifdef HIDUMPER_BUNDLEMANAGER_FRAMEWORK_ENABLE
    static bool GetApplicationInfoBase(int pid, AppExecFwk::ApplicationInfo &appInfo);
#endif
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
