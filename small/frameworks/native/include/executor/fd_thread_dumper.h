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
#ifndef FD_THREAD_DUMPER_H
#define FD_THREAD_DUMPER_H
#include <vector>
#include <string>
#include <memory>
#include <sstream>
#include <unistd.h>
#include <sys/syscall.h>
#include <dirent.h>
#include <set>
#include <queue>
#include <algorithm>
#include <limits>
#include <map>
#include "hidumper_executor.h"

namespace OHOS {
namespace HiviewDFX {

constexpr size_t FD_TOP_CNT = 10;
constexpr int FD_PATH_MAX = 4096;
constexpr size_t START_TIME_FIELD_INDEX = 19;
static const std::string STORAGE_PATH_PREFIX = "/data/storage/el";
static const size_t STORAGE_PATH_SIZE = STORAGE_PATH_PREFIX.size();

class FdThreadDumper : public HidumperExecutor {
public:
    FdThreadDumper();
    ~FdThreadDumper();

    DumpStatus PreExecute(const std::shared_ptr<DumperParameter> &parameter, StringMatrix dumpDatas) override;
    DumpStatus Execute() override;
    DumpStatus AfterExecute() override;

private:
    bool DumpFdInfo();
    void DumpThreadInfo();
    void DumpFdSummary(const std::vector<std::pair<std::string, int>>& topLinks,
                      const std::vector<std::pair<std::string, int>>& topTypes);
    void DumpFdTopInfo(const std::vector<std::pair<std::string, int>>& topLinks);
    void DumpFdDirInfo(const std::vector<std::pair<std::string, int>>& topTypes,
                       const std::map<std::string, std::unordered_map<std::string, int>>& typePaths);
    void DumpFdLinkCounts(const std::unordered_map<std::string, int>& linkCounts);
    std::string GetThreadStartTime(int pid, const std::string &tid);
    std::string GetThreadName(int pid, const std::string &tid);
    std::string GetFdLink(const std::string &linkPath);
    std::vector<std::string> GetFdLinks(int pid);
    std::string MaybeKnownType(const std::string &link);
    std::unordered_map<std::string, int> CountPaths(const std::vector<std::string>& links);
    std::vector<std::pair<std::string, int>> TopN(const std::unordered_map<std::string, int>& counter, size_t n);
    std::string GetSummary(const std::vector<std::pair<std::string, int>>& topLinks,
                                       const std::vector<std::pair<std::string, int>>& topTypes);
    std::string GetTopFdInfo(const std::vector<std::pair<std::string, int>>& topLinks);
    std::string GetTopDirInfo(const std::vector<std::pair<std::string, int>>& topTypes,
                                   const std::map<std::string, std::unordered_map<std::string, int>>& typePaths);
    std::vector<std::string> GetSubNodes(const std::string &path, bool digit);
    bool IsNumericStr(const std::string &str);
    size_t FindFdClusterStartIndex(const std::string &fullFileName);

private:
    StringMatrix dumpDatas_;
    bool isDumpFd_;
    bool isDumpThread_;
    bool isDumpFdThreadAll_;
    int processPid_;
};
} // namespace HiviewDFX
} // namespace OHOS
#endif
