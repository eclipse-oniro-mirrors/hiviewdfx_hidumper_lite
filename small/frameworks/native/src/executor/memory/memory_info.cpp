/*
* Copyright (C) 2021-2022 Huawei Device Co., Ltd.
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/
#include "executor/memory/memory_info.h"
#include <algorithm>
#include <atomic>
#include <sys/stat.h>
#include <dlfcn.h>
#include <cinttypes>
#include <fstream>
#include <numeric>
#include <sstream>
#include <iomanip>
#include <thread>

#include "dump_common_utils.h"
#include "dump_utils.h"
#include "executor/memory/get_cma_info.h"
#include "executor/memory/get_hardware_info.h"
#include "executor/memory/get_kernel_info.h"
#include "executor/memory/get_process_info.h"
#include "executor/memory/get_ram_info.h"
#include "executor/memory/memory_util.h"
#include "executor/memory/parse/parse_ashmem_info.h"
#include "executor/memory/parse/parse_dmabuf_info.h"
#include "executor/memory/parse/parse_meminfo.h"
#include "executor/memory/parse/parse_smaps_rollup_info.h"
#include "executor/memory/parse/parse_smaps_info.h"
#include "file_ex.h"
#include "hilog_wrapper.h"
#ifdef HIDUMPER_MEMMGR_ENABLE
#include "mem_mgr_constant.h"
#endif
#include "securec.h"
#include "string_ex.h"
#include "util/string_utils.h"
#include "util/file_utils.h"
#include "common/dumper_constant.h"

using namespace std;

namespace OHOS {
namespace HiviewDFX {
static const int PAGETAG_MIN_LEN = 2; // page tag min length
static const std::string UNKNOWN_PROCESS = "unknown";
static const std::string PRE_BLANK = "   ";
static const std::string END_BLANK = "  ";
static const std::string MEMORY_LINE = "-------------------------------[memory]-------------------------------";
std::atomic<bool> g_isDumpMem = true;
constexpr int SECOND_TO_MILLISECONDS = 1000;
constexpr int MAX_STARS_NUM = 20;
constexpr int ONE_STAR = 1;
constexpr int APP_UID = 20000;
constexpr int LINE_SPACING = 6;
constexpr int DMABUF_MAX_WIDTH = 33;

MemoryInfo::MemoryInfo()
{
    methodVec_.clear();
    methodVec_.push_back(make_pair(MEMINFO_PSS,
        bind(&MemoryInfo::SetPss, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_SHARED_CLEAN,
        bind(&MemoryInfo::SetSharedClean, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_SHARED_DIRTY,
        bind(&MemoryInfo::SetSharedDirty, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_PRIVATE_CLEAN,
        bind(&MemoryInfo::SetPrivateClean, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_PRIVATE_DIRTY,
        bind(&MemoryInfo::SetPrivateDirty, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_SWAP,
        bind(&MemoryInfo::SetSwap, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_SWAP_PSS,
        bind(&MemoryInfo::SetSwapPss, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_HEAP_SIZE,
        bind(&MemoryInfo::SetHeapSize, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_HEAP_ALLOC,
        bind(&MemoryInfo::SetHeapAlloc, this, placeholders::_1, placeholders::_2)));
    methodVec_.push_back(make_pair(MEMINFO_HEAP_FREE,
        bind(&MemoryInfo::SetHeapFree, this, placeholders::_1, placeholders::_2)));
}

MemoryInfo::~MemoryInfo()
{
}

void MemoryInfo::WriteStdout(const std::string& s)
{
    write(rawParamFd_, s.c_str(), s.size());
}

void MemoryInfo::ClearPreviousLines(int lineCount)
{
    for (int i = 0; i < lineCount; ++i) {
        WriteStdout("\033[F");
        WriteStdout("\033[2K");
    }
}

int MemoryInfo::CalculateStars(const std::vector<int>& pssValues, int currentPSS)
{
    if (pssValues.empty()) {
        return MAX_STARS_NUM;
    }
    auto [min_it, max_it] = std::minmax_element(pssValues.begin(), pssValues.end());
    int minPSS = *min_it;
    int maxPSS = *max_it;
    if (maxPSS == minPSS) {
        return MAX_STARS_NUM;
    }
    double stdValue = static_cast<double>(currentPSS - minPSS) / (maxPSS - minPSS);
    return static_cast<int>(stdValue * (MAX_STARS_NUM - ONE_STAR) + ONE_STAR);
}

std::string MemoryInfo::GenerateLine(const std::vector<int>& pssValues, int index)
{
    DUMPER_HILOGI(MODULE_SERVICE, "pss:%{public}d", pssValues[index]);
    int stars = CalculateStars(pssValues, pssValues[index]);
    const int TOTAL_WIDTH = 26;
    std::stringstream ss;
    ss << std::string(stars, '*')
       << std::string(TOTAL_WIDTH - stars, ' ')
       << "times" << (index + 1) << ": PSS=" << pssValues[index] << "kB\n";
    return ss.str();
}

std::string MemoryInfo::GenerateTimestamps(const std::vector<int>& pssValues)
{
    std::string endTime;
    DumpCommonUtils::GetDateAndTime(DumpCommonUtils::GetMilliseconds() / SECOND_TO_MILLISECONDS, endTime);
    int timeIndex = static_cast<int>(pssValues.size());
    int maxMem = *std::max_element(pssValues.begin(), pssValues.end());
    int changePss = 0;
    if (pssValues.size() > 1) {
        changePss = pssValues[pssValues.size() - 1] - pssValues[0];
    }

    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, startTime_);
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, endTime);
    string count = to_string(timeIndex);
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, count);
    string maxMemStr = to_string(maxMem) + "kB";
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, maxMemStr);
    string changePssStr = to_string(changePss) + "kB";
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, changePssStr);
    return "StartTime           EndTime              Count                MaxMem              Change\n" +
           startTime_ + endTime + " " + count + " " + maxMemStr + " " + changePssStr + "\n";
}

void MemoryInfo::CalculateMaxIdex(const std::vector<int>& pssValues, int *maxIndex)
{
    const int minIndex = 5;
    const int lastLines = 5;
    for (int i = minIndex; i < static_cast<int>(pssValues.size()) - lastLines; ++i) {
        if (pssValues[i] > pssValues[*maxIndex]) {
            *maxIndex = i;
        }
    }
}

void MemoryInfo::PrintMemoryInfo(const std::vector<int>& pssValues, int* prevLineCount)
{
    static int lastMaxIndex = -1;
    static int numHeaderLines = 2;
    const int minIndex = 5;
    const int lastLines = 5;
    const int thresholdLines = 6;
    const size_t maxPssValues = 10;
    int currentLineCount = 0;
    std::string output;

    output += GenerateTimestamps(pssValues);
    currentLineCount += numHeaderLines;

    if (pssValues.size() <= maxPssValues) {
        currentLineCount += static_cast<int>(pssValues.size());
        for (size_t i = 0; i < pssValues.size(); ++i) {
            output += GenerateLine(pssValues, static_cast<int>(i));
        }
    } else {
        int maxIndex = 0;
        CalculateMaxIdex(pssValues, &maxIndex);
        for (int i = 0; i < minIndex; ++i) {
            output += GenerateLine(pssValues, i);
            currentLineCount++;
        }

        if (maxIndex >= minIndex && maxIndex < static_cast<int>(pssValues.size()) - lastLines) {
            if (maxIndex > minIndex) {
                output += "...\n";
                currentLineCount++;
            }

            output += GenerateLine(pssValues, maxIndex);
            currentLineCount++;

            if (maxIndex < static_cast<int>(pssValues.size()) - thresholdLines) {
                output += "...\n";
                currentLineCount++;
            }

            lastMaxIndex = maxIndex;
        } else {
            output += "...\n";
            currentLineCount++;
        }

        for (int i = static_cast<int>(pssValues.size()) - lastLines; i < static_cast<int>(pssValues.size()); ++i) {
            output += GenerateLine(pssValues, i);
            currentLineCount++;
        }
    }

    if (*prevLineCount > 0) {
        ClearPreviousLines(*prevLineCount);
    }

    WriteStdout(output);
    *prevLineCount = currentLineCount;
}

void MemoryInfo::SetReceivedSigInt(bool isReceivedSigInt)
{
    DUMPER_HILOGI(MODULE_SERVICE, "isReceivedSigInt: %{public}d", isReceivedSigInt);
    g_isDumpMem = !isReceivedSigInt;
}

void MemoryInfo::RedirectMemoryInfo(int timeIndex, StringMatrix result)
{
    mkdir("/data/log/hidumper", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    int redirectFd = DumpUtils::FdToWrite("/data/log/hidumper/record_mem.txt");
    if (redirectFd <= -1) {
        DUMPER_HILOGE(MODULE_COMMON, "write to record_mem.txt failed");
        return;
    }
    fdsan_exchange_owner_tag(redirectFd, 0, FDTAG);
    string timeTitle = "\ntimes:" + to_string(timeIndex) + "\n";
    write(redirectFd, timeTitle.c_str(), strlen(timeTitle.c_str()));
    for (size_t i = 0; i < result->size(); i++) {
        std::vector<std::string> line = result->at(i);
        for (size_t j = 0; j < line.size(); j++) {
            std::string str = line[j];
            if ((j == (line.size() - 1)) && (str.find("\n") == std::string::npos)) {
                str = str + "\n";
            }
            if (write(redirectFd, str.c_str(), strlen(str.c_str())) == -1) {
                DUMPER_HILOGE(MODULE_COMMON, "write to rawParamFd failed, errno: %{public}d", errno);
            }
        }
    }
    fdsan_close_with_tag(redirectFd, FDTAG);
}

void MemoryInfo::GetMemoryInfoByTimeInterval(int fd, const int32_t &pid, const int32_t &timeInterval)
{
    std::lock_guard<std::mutex> lock(timeIntervalMutex_);
    rawParamFd_ = fd;
    (void)dprintf(rawParamFd_, "%s\n\n", MEMORY_LINE.c_str());
    DumpCommonUtils::GetDateAndTime(DumpCommonUtils::GetMilliseconds() / SECOND_TO_MILLISECONDS, startTime_);
    DUMPER_HILOGI(MODULE_SERVICE, "GetMemoryInfoByTimeInterval timeInterval:%{public}d", timeInterval);
    std::vector<int> pssValues;
    int prevLineCount = 0;
    while (g_isDumpMem) {
        StringMatrix result = std::make_shared<std::vector<std::vector<std::string>>>();
        GetMemoryInfoByPid(pid, result, false, false, false);
        pssValues.push_back(static_cast<int>(currentPss_));
        PrintMemoryInfo(pssValues, &prevLineCount);
        RedirectMemoryInfo(static_cast<int>(pssValues.size()), result);
        sleep(timeInterval);
    }
    g_isDumpMem = true;
    DUMPER_HILOGI(MODULE_SERVICE, "GetMemoryInfoByTimeInterval timeInterval:%{public}d end", timeInterval);
}

namespace {
struct SmapsSummary {
    uint64_t pss = 0;
    uint64_t rss = 0;
    uint64_t swap = 0;
    uint64_t swapPss = 0;
    uint64_t privateClean = 0;
    uint64_t privateDirty = 0;
    uint64_t sharedClean = 0;
    uint64_t sharedDirty = 0;
};

bool ReadSmapsSummary(int32_t pid, SmapsSummary& summary)
{
    std::string path = "/proc/" + std::to_string(pid) + "/smaps";
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        auto pos = line.find(':');
        if (pos == std::string::npos) {
            continue;
        }
        std::istringstream iss(line.substr(pos + 1));
        uint64_t value = 0;
        if (!(iss >> value)) {
            continue;
        }
        std::string key = line.substr(0, pos);
        if (key == "Pss") {
            summary.pss += value;
        } else if (key == "Rss") {
            summary.rss += value;
        } else if (key == "Swap") {
            summary.swap += value;
        } else if (key == "SwapPss") {
            summary.swapPss += value;
        } else if (key == "Private_Clean") {
            summary.privateClean += value;
        } else if (key == "Private_Dirty") {
            summary.privateDirty += value;
        } else if (key == "Shared_Clean") {
            summary.sharedClean += value;
        } else if (key == "Shared_Dirty") {
            summary.sharedDirty += value;
        }
    }
    return true;
}

void AppendMetricRow(MemoryInfo::StringMatrix result, const std::string& name, uint64_t value)
{
    vector<string> row;
    string title = name + ":";
    StringUtils::GetInstance().SetWidth(20, ' ', false, title);
    row.push_back(title + to_string(value) + MemoryUtil::GetInstance().KB_UNIT_);
    result->push_back(row);
}
} // namespace

bool MemoryInfo::GetMemoryInfoByPid(const int32_t &pid, StringMatrix result,
                                    bool showAshmem, bool showDmaBuf, bool showGpumem)
{
    std::lock_guard<std::mutex> lock(mutex_);
    SmapsSummary summary;
    if (!ReadSmapsSummary(pid, summary)) {
        DUMPER_HILOGE(MODULE_SERVICE, "read smaps fail, pid:%{public}d", pid);
        return false;
    }
    currentPss_ = summary.pss;

    string name = GetProcName(pid);
    vector<string> titleRow;
    titleRow.push_back("Process Memory Information : " + name + "(pid = " + to_string(pid) + ")");
    result->push_back(titleRow);

    AppendMetricRow(result, "VmPeak", GetProcValue(pid, "VmPeak"));
    AppendMetricRow(result, "VmSize", GetProcValue(pid, "VmSize"));
    AppendMetricRow(result, "VmRSS", GetProcValue(pid, "VmRSS"));
    AppendMetricRow(result, "RssAnon", GetProcValue(pid, "RssAnon"));
    AppendMetricRow(result, "RssFile", GetProcValue(pid, "RssFile"));
    AppendMetricRow(result, "RssShmem", GetProcValue(pid, "RssShmem"));
    AppendMetricRow(result, "VmData", GetProcValue(pid, "VmData"));
    AppendMetricRow(result, "VmSwap", GetProcValue(pid, "VmSwap"));

    AppendMetricRow(result, "Rss(total)", summary.rss);
    AppendMetricRow(result, "Pss(total)", summary.pss);
    AppendMetricRow(result, "Private_Clean", summary.privateClean);
    AppendMetricRow(result, "Private_Dirty", summary.privateDirty);
    AppendMetricRow(result, "Shared_Clean", summary.sharedClean);
    AppendMetricRow(result, "Shared_Dirty", summary.sharedDirty);
    AppendMetricRow(result, "Swap", summary.swap);
    AppendMetricRow(result, "SwapPss", summary.swapPss);

    GetPurgByPid(pid, result);
    GetAshmem(pid, result, showAshmem);
    GetDmaBuf(pid, result, showDmaBuf);
    GetGpumem(pid, result, showGpumem);
    return true;
}

string MemoryInfo::AddKbUnit(const uint64_t &value) const
{
    return to_string(value) + MemoryUtil::GetInstance().KB_UNIT_;
}

bool MemoryInfo::GetSmapsInfoNoPid(const int32_t &pid, GroupMap &result)
{
    GroupMap NativeMap;
    unique_ptr<ParseSmapsInfo> parseSmapsInfo = make_unique<ParseSmapsInfo>();
    return parseSmapsInfo->GetInfo(MemoryFilter::NOT_SPECIFIED_PID, pid, NativeMap, result);
}

bool MemoryInfo::GetMeminfo(ValueMap &result)
{
    unique_ptr<ParseMeminfo> parseMeminfo = make_unique<ParseMeminfo>();
    return parseMeminfo->GetMeminfo(result);
}

bool MemoryInfo::GetHardWareUsage(StringMatrix result)
{
    uint64_t value;
    unique_ptr<GetHardwareInfo> getHardwareInfo = make_unique<GetHardwareInfo>();
    if (getHardwareInfo->GetHardwareUsage(value)) {
        string title = "Hardware Usage:";
        StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, title);
        SaveStringToFd(rawParamFd_, title + AddKbUnit(value) + "\n");
        return true;
    }
    return false;
}

bool MemoryInfo::GetCMAUsage(StringMatrix result)
{
    uint64_t value = 0;
    unique_ptr<GetCMAInfo> getCMAInfo = make_unique<GetCMAInfo>();
    bool success = getCMAInfo->GetUsed(value);
    if (success) {
        string title = "CMA Usage:";
        StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, title);
        SaveStringToFd(rawParamFd_, title + AddKbUnit(value) + "\n");
    }
    return success;
}

bool MemoryInfo::GetKernelUsage(const ValueMap &infos, StringMatrix result)
{
    uint64_t value = 0;
    unique_ptr<GetKernelInfo> getGetKernelInfo = make_unique<GetKernelInfo>();
    bool success = getGetKernelInfo->GetKernel(infos, value);
    if (success) {
        string title = "Kernel Usage:";
        StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, title);
        SaveStringToFd(rawParamFd_, title + AddKbUnit(value) + "\n");
    }
    return success;
}

void MemoryInfo::GetProcesses(const GroupMap &infos, StringMatrix result)
{
    uint64_t value = 0;

    unique_ptr<GetProcessInfo> getProcessInfo = make_unique<GetProcessInfo>();
    value = getProcessInfo->GetProcess(infos);

    string title = "Processes Usage:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, title);
    SaveStringToFd(rawParamFd_, title + AddKbUnit(value) + "\n");
}

void MemoryInfo::GetPssTotal(const GroupMap &infos, StringMatrix result)
{
    SaveStringToFd(rawParamFd_, "Total Pss by Category:\n");
    vector<pair<string, uint64_t>> filePage;
    vector<pair<string, uint64_t>> anonPage;
    for (const auto &info : infos) {
        vector<string> pageTag;
        StringUtils::GetInstance().StringSplit(info.first, "#", pageTag);
        if (pageTag.size() < PAGETAG_MIN_LEN) {
            DUMPER_HILOGE(MODULE_COMMON, "Infos are invalid, info.first: %{public}s", info.first.c_str());
            continue;
        }
        string group = pageTag[1];
        auto &valueMap = info.second;
        uint64_t pssValue = 0;
        for (const auto &str : MemoryFilter::GetInstance().CALC_PSS_TOTAL_) {
            auto it = valueMap.find(str);
            if (it != valueMap.end()) {
                pssValue += it->second;
            }
        }

        if (pageTag[0] == MemoryFilter::GetInstance().FILE_PAGE_TAG) {
            filePage.push_back(make_pair(group, pssValue));
        } else {
            anonPage.push_back(make_pair(group, pssValue));
        }
    }
    PairToStringMatrix(MemoryFilter::GetInstance().FILE_PAGE_TAG, filePage, result);
    PairToStringMatrix(MemoryFilter::GetInstance().ANON_PAGE_TAG, anonPage, result);

    vector<pair<string, uint64_t>> gpuValue;
    gpuValue.push_back(make_pair(MemoryFilter::GetInstance().GL_OUT_LABEL, totalGL_));
    PairToStringMatrix(MemoryFilter::GetInstance().GPU_TAG, gpuValue, result);

    vector<pair<string, uint64_t>> graphValue;
    graphValue.push_back(make_pair(MemoryFilter::GetInstance().GRAPH_OUT_LABEL, totalGraph_));
    PairToStringMatrix(MemoryFilter::GetInstance().GRAPH_OUT_LABEL, graphValue, result);

    vector<pair<string, uint64_t>> dmaValue;
    dmaValue.push_back(make_pair(MemoryFilter::GetInstance().DMA_OUT_LABEL, totalDma_));
    PairToStringMatrix(MemoryFilter::GetInstance().DMA_TAG, dmaValue, result);
}

void MemoryInfo::PairToStringMatrix(const string &titleStr, vector<pair<string, uint64_t>> &vec, StringMatrix result)
{
    uint64_t totalPss = accumulate(vec.begin(), vec.end(), (uint64_t)0, [] (uint64_t a, pair<string, uint64_t> &b) {
        return a + b.second;
    });
    SaveStringToFd(rawParamFd_, titleStr + "(" + AddKbUnit(totalPss) + "):\n");

    std::sort(vec.begin(), vec.end(),
        [] (pair<string, uint64_t> &left, pair<string, uint64_t> &right) {
        return right.second < left.second;
    });
    for (const auto &pair : vec) {
        string pssStr = AddKbUnit(pair.second);
        StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, pssStr);
        SaveStringToFd(rawParamFd_, pssStr + " : " + pair.first + "\n");
    }
}

void MemoryInfo::GetRamUsage(const GroupMap &smapsinfos, const ValueMap &meminfo, StringMatrix result)
{
    unique_ptr<GetRamInfo> getRamInfo = make_unique<GetRamInfo>();
    GetRamInfo::Ram ram = getRamInfo->GetRam(smapsinfos, meminfo);

    string totalTitle = "Total RAM:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, totalTitle);
    SaveStringToFd(rawParamFd_, totalTitle + AddKbUnit(ram.total) + "\n");

    string freeTitle = "Free RAM:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, freeTitle);
    SaveStringToFd(rawParamFd_, freeTitle + AddKbUnit(ram.free) +
        " (" + to_string(ram.cachedInfo) + " cached + " + to_string(ram.freeInfo) + " free)\n");

    string usedTitle = "Used RAM:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, usedTitle);
    SaveStringToFd(rawParamFd_, usedTitle + AddKbUnit(ram.used) +
        " (" + to_string(ram.totalPss) + " total pss + " + to_string(ram.kernelUsed) + " kernel)\n");

    string lostTitle = "Lost RAM:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, lostTitle);
    SaveStringToFd(rawParamFd_, lostTitle + to_string(ram.lost) + MemoryUtil::GetInstance().KB_UNIT_ + "\n");
}

void MemoryInfo::GetPurgTotal(const ValueMap &meminfo, StringMatrix result)
{
    SaveStringToFd(rawParamFd_, "Total Purgeable:\n");

    uint64_t purgSumTotal = 0;
    uint64_t purgPinTotal = 0;
    auto purgSumActive = meminfo.find(MemoryFilter::GetInstance().PURG_SUM[0]);
    auto purgSumInactive = meminfo.find(MemoryFilter::GetInstance().PURG_SUM[1]);
    auto purgPinPined = meminfo.find(MemoryFilter::GetInstance().PURG_PIN[0]);
    if (purgSumActive == meminfo.end() || purgSumInactive == meminfo.end() || purgPinPined == meminfo.end()) {
        DUMPER_HILOGE(MODULE_SERVICE, "fail to get purg info \n");
    } else {
        purgSumTotal = purgSumActive->second + purgSumInactive->second;
        purgPinTotal = purgPinPined->second;
    }

    string totalPurgSumTitle = "Total PurgSum:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, totalPurgSumTitle);
    SaveStringToFd(rawParamFd_, totalPurgSumTitle + AddKbUnit(purgSumTotal) + "\n");

    string totalPurgPinTitle = "Total PurgPin:";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, totalPurgPinTitle);
    SaveStringToFd(rawParamFd_, totalPurgPinTitle + AddKbUnit(purgPinTotal) + "\n");
}

void MemoryInfo::GetPurgByPid(const int32_t &pid, StringMatrix result)
{
    AddBlankLine(result);
    vector<string> title;
    title.push_back("Purgeable:");
    result->push_back(title);

    vector<string> purgSum;
    string purgSumTitle = MemoryFilter::GetInstance().PURGSUM_OUT_LABEL + ":";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, purgSumTitle);
    purgSum.push_back(purgSumTitle);
    purgSum.push_back(AddKbUnit(GetProcValue(pid, MemoryFilter::GetInstance().PURGSUM_OUT_LABEL)));
    result->push_back(purgSum);

    vector<string> purgPin;
    string purgPinTitle = MemoryFilter::GetInstance().PURGPIN_OUT_LABEL + ":";
    StringUtils::GetInstance().SetWidth(RAM_WIDTH_, BLANK_, false, purgPinTitle);
    purgPin.push_back(purgPinTitle);
    purgPin.push_back(AddKbUnit(GetProcValue(pid, MemoryFilter::GetInstance().PURGPIN_OUT_LABEL)));
    result->push_back(purgPin);
}

void MemoryInfo::GetAshmem(const int32_t &pid, StringMatrix result, bool showAshmem)
{
    std::pair<int, std::vector<std::string>> ashmemInfo;
    unique_ptr<ParseAshmemInfo> parseAshmeminfo = make_unique<ParseAshmemInfo>();
    if (!parseAshmeminfo->GetAshmemInfo(pid, ashmemInfo)) {
        DUMPER_HILOGE(MODULE_SERVICE, "GetAshmemInfo error");
        return;
    }
    if (ashmemInfo.second.size() == 0) {
        DUMPER_HILOGE(MODULE_SERVICE, "no detail ashmem info.");
        return;
    }
    AddBlankLine(result);
    vector<string> title;
    title.push_back("Ashmem:");
    result->push_back(title);
    vector<string> totalAshmemVec;
    totalAshmemVec.push_back("Total Ashmem:" + to_string(ashmemInfo.first) + MemoryUtil::GetInstance().KB_UNIT_);
    result->push_back(totalAshmemVec);
    if (showAshmem) {
        for (auto detailInfo : ashmemInfo.second) {
            vector<string> tempResult;
            tempResult.push_back(detailInfo);
            result->push_back(tempResult);
        }
    }
}

bool MemoryInfo::GetDmaBuf(const int32_t &pid, StringMatrix result, bool showDmaBuf)
{
    if (!showDmaBuf) {
        DUMPER_HILOGD(MODULE_SERVICE, "showDmaBuf is false, skip GetDmaBuf");
        return false;
    }
    const std::vector<std::string> showTitles = { "Process", "pid", "fd", "size_bytes", "ino", "exp_pid",
        "exp_task_comm", "buf_name", "exp_name", "buf_type", "leak_type" };
    return GetDmaBufByProc(pid, result, showTitles);
}
bool MemoryInfo::GetGpumem(const int32_t &pid, StringMatrix result, bool showGpumem)
{
    if (!showGpumem) {
        DUMPER_HILOGD(MODULE_SERVICE, "showGpumem is false, skip GetGpumem");
        return false;
    }
    AddBlankLine(result);
    std::vector<std::string> hint;
    hint.push_back("Error [RUNTIME]: gpumem plugin is not loaded; --show-gpumem is unavailable.");
    result->push_back(hint);
    DUMPER_HILOGE(MODULE_SERVICE, "CollectGpumem returns empty, gpumem plugin not loaded");
    return false;
    return true;
}

bool MemoryInfo::GetDmaBufByProc(const int32_t &pid, StringMatrix result, const std::vector<std::string>& showTitles)
{
    int32_t uid = GetProcUid(pid);
    if (uid < APP_UID) {
        DUMPER_HILOGD(MODULE_SERVICE, "Uid Verification failed for uid: %{public}d", uid);
        return false;
    }
    std::vector<std::string> dmabufInfo;
    std::vector<std::string> titles;
    std::unordered_map<std::string, int> headerMap;
    std::vector<int> columnWidths;
    unique_ptr<ParseDmaBufInfo> parseDmaBufInfo = make_unique<ParseDmaBufInfo>();
    if (!parseDmaBufInfo->GetDmaBufInfo(pid, dmabufInfo, headerMap, columnWidths, titles)) {
        DUMPER_HILOGE(MODULE_SERVICE, "GetDmaBufInfo error");
        return false;
    }
    for (const auto& dmabuf : dmabufInfo) {
        std::istringstream ss(dmabuf);
        std::ostringstream oss;
        for (const auto& title : titles) {
            std::string value;
            if (!(ss >> value)) {
                value = "NULL";
            }
            if (std::find(showTitles.begin(), showTitles.end(), title) == showTitles.end()) {
                continue;
            }
            int width = std::min(columnWidths[headerMap[title]], DMABUF_MAX_WIDTH);
            oss << std::left << std::setw(width + LINE_SPACING) << value << END_BLANK;
        }
        vector<string> tempResult;
        tempResult.push_back(oss.str());
        result->push_back(tempResult);
    }
    return true;
}

void MemoryInfo::GetRamCategory(const GroupMap &smapsInfos, const ValueMap &meminfos, StringMatrix result)
{
    SaveStringToFd(rawParamFd_, "Total RAM by Category:\n");

    bool hardWareSuccess = GetHardWareUsage(result);
    if (!hardWareSuccess) {
        DUMPER_HILOGE(MODULE_SERVICE, "Get hardWare usage fail.\n");
    }

    bool cmaSuccess = GetCMAUsage(result);
    if (!cmaSuccess) {
        DUMPER_HILOGE(MODULE_SERVICE, "Get CMA fail.\n");
    }

    bool kernelSuccess = GetKernelUsage(meminfos, result);
    if (!kernelSuccess) {
        DUMPER_HILOGE(MODULE_SERVICE, "Get kernel usage fail.\n");
    }

    GetProcesses(smapsInfos, result);
}

void MemoryInfo::AddBlankLine(StringMatrix result)
{
    vector<string> blank;
    blank.push_back("\n");
    result->push_back(blank);
}

string MemoryInfo::GetProcName(const int32_t &pid)
{
    string procName = UNKNOWN_PROCESS;
    DumpCommonUtils::GetProcessNameByPid(pid, procName);
    if (procName == UNKNOWN_PROCESS) {
        string path = "/proc/" + to_string(pid) + "/status";
        procName = FileUtils::GetInstance().GetProcValue(pid, path, "Name");
    }
    return procName;
}

int32_t MemoryInfo::GetProcUid(const int32_t &pid)
{
    string path = "/proc/" + to_string(pid) + "/status";
    string procUid = FileUtils::GetInstance().GetProcValue(pid, path, "Uid");
    if (procUid == UNKNOWN_PROCESS) {
        DUMPER_HILOGE(MODULE_SERVICE, "GetProcUid failed");
        return -1;
    }
    istringstream uid_stream(procUid);
    string value;
    uid_stream >> value;
    int uid = -1;
    if (!StrToInt(value, uid)) {
        DUMPER_HILOGE(MODULE_COMMON, "StrToInt failed, value: %{public}s", value.c_str());
        return -1;
    }
    return static_cast<int32_t>(uid);
}

uint64_t MemoryInfo::GetProcValue(const int32_t &pid, const string& key)
{
    string path = "/proc/" + to_string(pid) + "/status";
    std::string value = FileUtils::GetInstance().GetProcValue(pid, path, key);
    if (value == UNKNOWN_PROCESS) {
        DUMPER_HILOGE(MODULE_SERVICE, "GetProcStatusValue failed");
        return 0;
    }
    int number = 0;
    value = value.substr(0, value.size() - 3); // 3: ' kB'
    if (value.find_last_of(' ') != std::string::npos) {
        value = value.substr(value.find_last_of(' ') + 1);
    }
    if (!StrToInt(value, number)) {
        DUMPER_HILOGE(MODULE_COMMON, "StrToInt failed, value: %{public}s", value.c_str());
        return 0;
    }
    return static_cast<uint64_t>(number);
}

#ifdef HIDUMPER_MEMMGR_ENABLE
string MemoryInfo::GetProcessAdjLabel(const int32_t pid)
{
    string adjLabel = Memory::RECLAIM_PRIORITY_UNKNOWN_DESC;
    string fillPath = "/proc/" + to_string(pid) + "/oom_score_adj";
    if (!DumpUtils::PathIsValid(fillPath)) {
        DUMPER_HILOGE(MODULE_COMMON, "GetProcessAdjLabel leave|false, PathIsValid");
        return adjLabel;
    }
    auto fp = fopen(fillPath.c_str(), "rb");
    if (fp == nullptr) {
        DUMPER_HILOGE(MODULE_COMMON, "Open oom_score_adj failed.");
        return adjLabel;
    }
    constexpr int bufSize = 128; // 128: buf size
    char buf[bufSize] = {0};
    size_t readSum = fread(buf, 1, bufSize, fp);
    (void)fclose(fp);
    fp = nullptr;
    if (readSum < 1) {
        DUMPER_HILOGE(MODULE_COMMON, "Read oom_score_adj failed.");
        return adjLabel;
    }
    int value = Memory::RECLAIM_PRIORITY_UNKNOWN;
    std::string label(buf);
    if (label.empty()) {
        DUMPER_HILOGE(MODULE_COMMON, "label is empty.");
        return adjLabel;
    }
    if (!StrToInt(label.substr(0, label.size() - 1), value)) {
        DUMPER_HILOGE(MODULE_COMMON, "StrToInt failed.");
        return adjLabel;
    }
    adjLabel = Memory::GetReclaimPriorityString(value);
    return adjLabel;
}
#endif

int MemoryInfo::GetScoreAdj(const int32_t pid)
{
    string filePath = "/proc/" + to_string(pid) + "/oom_score_adj";
    if (!DumpUtils::PathIsValid(filePath)) {
        DUMPER_HILOGE(MODULE_COMMON, "GetScoreAdj leave|false, PathIsValid");
        return -1;
    }
    std::string content = "-1";
    OHOS::LoadStringFromFile(filePath, content);
    int value = -1;
    if (!StrToInt(content.substr(0, content.size() - 1), value)) {
        DUMPER_HILOGE(MODULE_COMMON, "GetScoreAdj|StrToInt failed.");
        return -1;
    }
    return value;
}

bool MemoryInfo::GetPids()
{
    bool success = DumpCommonUtils::GetUserPids(pids_);
    if (!success) {
        DUMPER_HILOGE(MODULE_SERVICE, "GetPids error");
    }
    return success;
}

uint64_t MemoryInfo::GetVss(const int32_t &pid)
{
    string path = "/proc/" + to_string(pid) + "/statm";
    uint64_t res = 0;
    bool ret = FileUtils::GetInstance().LoadStringFromProcCb(path, true, true, [&](const string& line) -> void {
        if (!line.empty()) {
            uint64_t tempValue = 0;
            int retScanf = sscanf_s(line.c_str(), "%llu^*", &tempValue);
            if (retScanf != -1) {
                res = tempValue * VSS_BIT;
            } else {
                DUMPER_HILOGE(MODULE_SERVICE, "GetVss error! pid = %{public}d", pid);
            }
        }
    });
    if (!ret) {
        return 0;
    }
    return res;
}

bool MemoryInfo::GetMemByProcessPid(const int32_t &pid, MemInfoData::MemUsage &usage)
{
    bool success = false;
    MemInfoData::MemInfo memInfo;
    unique_ptr<ParseSmapsRollupInfo> getSmapsRollup = make_unique<ParseSmapsRollupInfo>();
    if (getSmapsRollup->GetMemInfo(pid, memInfo)) {
        if (!dumpPrune_) {
            usage.vss = GetVss(pid);
            usage.uss = memInfo.privateClean + memInfo.privateDirty;
            usage.rss = memInfo.rss;
        }
        usage.pss = memInfo.pss;
        usage.swapPss = memInfo.swapPss;
        usage.name = GetProcName(pid);
        usage.pid = pid;
#ifdef HIDUMPER_MEMMGR_ENABLE
        usage.adjLabel = GetProcessAdjLabel(pid);
#endif
        success = true;
    }

    return success;
}

void MemoryInfo::MemUsageToMatrix(const MemInfoData::MemUsage &memUsage, StringMatrix result)
{
    string pid = to_string(memUsage.pid);
    StringUtils::GetInstance().SetWidth(PID_WIDTH_, BLANK_, true, pid);

    uint64_t pss = memUsage.pss + memUsage.swapPss;
    string totalPss = to_string(pss) + "(" + to_string(memUsage.swapPss) + " in SwapPss) kB";
    StringUtils::GetInstance().SetWidth(PSS_WIDTH_, BLANK_, false, totalPss);

    uint64_t gl = memUsage.gl;
    string unMappedGL = AddKbUnit(gl);
    StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedGL);

    string name = "    " + memUsage.name;
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, name);

    if (!dumpPrune_) {
        uint64_t vss = memUsage.vss;
        string totalVss = AddKbUnit(vss);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalVss);

        uint64_t rss = memUsage.rss;
        string totalRss = AddKbUnit(rss);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalRss);

        uint64_t uss = memUsage.uss;
        string totalUss = AddKbUnit(uss);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalUss);

        uint64_t graph = memUsage.graph;
        string unMappedGraph = AddKbUnit(graph);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedGraph);

        uint64_t dma = memUsage.dma;
        string unMappedDma = AddKbUnit(dma);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedDma);

        uint64_t purgSum = memUsage.purgSum;
        string unMappedPurgSum = AddKbUnit(purgSum);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedPurgSum);

        uint64_t purgPin = memUsage.purgPin;
        string unMappedPurgPin = AddKbUnit(purgPin);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedPurgPin);

        (void)dprintf(rawParamFd_, "%s %s %s %s %s %s %s %s %s %s %s\n", pid.c_str(),
            totalPss.c_str(), totalVss.c_str(), totalRss.c_str(), totalUss.c_str(),
            unMappedGL.c_str(), unMappedGraph.c_str(), unMappedDma.c_str(), unMappedPurgSum.c_str(),
            unMappedPurgPin.c_str(), name.c_str());
    } else {
        int scoreAdj = GetScoreAdj(memUsage.pid);
        string scoreAdjStr = to_string(scoreAdj);
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, scoreAdjStr);

        (void)dprintf(rawParamFd_, "%s %s %s %s %s\n", pid.c_str(),
            totalPss.c_str(), unMappedGL.c_str(), scoreAdjStr.c_str(), name.c_str());
    }
}

void MemoryInfo::AddMemByProcessTitle(StringMatrix result, string sortType)
{
    string processTitle = "Total Memory Usage by " + sortType + ":";
    (void)dprintf(rawParamFd_, "%s\n", processTitle.c_str());

    string pid = "PID";
    StringUtils::GetInstance().SetWidth(PID_WIDTH_, BLANK_, true, pid);

    string totalPss = "Total Pss(xxx in SwapPss)";
    StringUtils::GetInstance().SetWidth(PSS_WIDTH_, BLANK_, false, totalPss);

    string unMappedGL = MemoryFilter::GetInstance().GL_OUT_LABEL;
    StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedGL);

    string name = "    Name";
    StringUtils::GetInstance().SetWidth(NAME_WIDTH_, BLANK_, true, name);

    if (!dumpPrune_) {
        string totalVss = "Total Vss";
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalVss);

        string totalRss = "Total Rss";
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalRss);

        string totalUss = "Total Uss";
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, totalUss);

        string unMappedGraph = MemoryFilter::GetInstance().GRAPH_OUT_LABEL;
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedGraph);

        string unMappedDma = MemoryFilter::GetInstance().DMA_OUT_LABEL;
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedDma);

        string unMappedPurgSum = MemoryFilter::GetInstance().PURGSUM_OUT_LABEL;
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedPurgSum);

        string unMappedPurgPin = MemoryFilter::GetInstance().PURGPIN_OUT_LABEL;
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, unMappedPurgPin);

        (void)dprintf(rawParamFd_, "%s %s %s %s %s %s %s %s %s %s %s\n", pid.c_str(),
            totalPss.c_str(), totalVss.c_str(), totalRss.c_str(), totalUss.c_str(),
            unMappedGL.c_str(), unMappedGraph.c_str(), unMappedDma.c_str(), unMappedPurgSum.c_str(),
            unMappedPurgPin.c_str(), name.c_str());
    } else {
        string adjLabel = "AdjLabel";
        StringUtils::GetInstance().SetWidth(KB_WIDTH_, BLANK_, false, adjLabel);

        (void)dprintf(rawParamFd_, "%s %s %s %s %s\n", pid.c_str(),
            totalPss.c_str(), unMappedGL.c_str(), adjLabel.c_str(), name.c_str());
    }
}

bool MemoryInfo::GetMemoryInfoInit(StringMatrix result)
{
    if (!isReady_) {
        memUsages_.clear();
        pids_.clear();
        totalGL_ = 0;
        totalGraph_ = 0;
        AddMemByProcessTitle(result, "PID");
        if (!GetPids()) {
            DUMPER_HILOGE(MODULE_SERVICE, "GetPids error!\n");
            return false;
        }
        isReady_ = true;
    }
    return true;
}

void MemoryInfo::GetMemoryUsageInfo(StringMatrix result)
{
    MemInfoData::MemUsage usage;
    MemoryUtil::GetInstance().InitMemUsage(usage);
    for (auto pid : pids_) {
        if (GetMemByProcessPid(pid, usage)) {
            memUsages_.push_back(usage);
            adjMemResult_[usage.adjLabel].push_back(usage);
            totalGL_ += usage.gl;
            totalGraph_ += usage.graph;
            totalDma_ += usage.dma;
            MemUsageToMatrix(usage, result);
        } else {
            DUMPER_HILOGE(MODULE_SERVICE, "Get smaps_rollup error! pid = %{public}d\n", static_cast<int>(pid));
        }
    }
}

DumpStatus MemoryInfo::GetMemoryInfoNoPid(int fd, StringMatrix result)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rawParamFd_ = fd;
    (void)dprintf(rawParamFd_, "%s\n", MEMORY_LINE.c_str());
    if (!GetMemoryInfoInit(result)) {
        return DUMP_FAIL;
    }

    if (!dumpSmapsOnStart_) {
        dumpSmapsOnStart_ = true;
        std::promise<GroupMap> promise;
        fut_ = promise.get_future();
        std::thread([promise = std::move(promise), this]() mutable {
            GroupMap groupMap;
            std::vector<int32_t> pids(this->pids_);
            for (auto pid : pids) {
                GetSmapsInfoNoPid(pid, groupMap);
            }
            promise.set_value(groupMap);
            }).detach();
    }
    GetMemoryUsageInfo(result);
    return DealResult(result);
}

DumpStatus MemoryInfo::GetMemoryInfoPrune(int fd, StringMatrix result)
{
    std::lock_guard<std::mutex> lock(mutex_);
    rawParamFd_ = fd;
    dumpPrune_ = true;
    (void)dprintf(rawParamFd_, "%s\n", MEMORY_LINE.c_str());
    if (!GetMemoryInfoInit(result)) {
        return DUMP_FAIL;
    }
    GetMemoryUsageInfo(result);
    return DUMP_OK;
}

DumpStatus MemoryInfo::DealResult(StringMatrix result)
{
    ValueMap meminfoResult;
    if (!GetMeminfo(meminfoResult)) {
        DUMPER_HILOGE(MODULE_SERVICE, "Get meminfo error\n");
        return DUMP_FAIL;
    }

    GetSortedMemoryInfoNoPid(result);
    SaveStringToFd(rawParamFd_, "\n");
#ifdef HIDUMPER_MEMMGR_ENABLE
    GetMemoryByAdj(result);
#endif
    SaveStringToFd(rawParamFd_, "\n");

    GroupMap smapsResult = fut_.get();

    GetPssTotal(smapsResult, result);
    SaveStringToFd(rawParamFd_, "\n");

    GetRamUsage(smapsResult, meminfoResult, result);
    SaveStringToFd(rawParamFd_, "\n");

    GetRamCategory(smapsResult, meminfoResult, result);
    SaveStringToFd(rawParamFd_, "\n");

    GetPurgTotal(meminfoResult, result);

    isReady_ = false;
    dumpSmapsOnStart_ = false;
    memUsages_.clear();
    smapsResult.clear();
    return DUMP_OK;
}

void MemoryInfo::GetSortedMemoryInfoNoPid(StringMatrix result)
{
    SaveStringToFd(rawParamFd_, "\n");
    AddMemByProcessTitle(result, "Size");

    std::sort(memUsages_.begin(), memUsages_.end(),
        [] (MemInfoData::MemUsage &left, MemInfoData::MemUsage &right) {
        if (right.pss + right.swapPss != left.pss + left.swapPss) {
            return right.pss + right.swapPss < left.pss + left.swapPss;
        }
        if (right.vss != left.vss) {
            return right.vss < left.vss;
        }
        if (right.rss != left.rss) {
            return right.rss < left.rss;
        }
        if (right.uss != left.uss) {
            return right.uss < left.uss;
        }
        return right.pid < left.pid;
    });

    for (auto &memUsage : memUsages_) {
        MemUsageToMatrix(memUsage, result);
    }
}

#ifdef HIDUMPER_MEMMGR_ENABLE
void MemoryInfo::GetMemoryByAdj(StringMatrix result)
{
    SaveStringToFd(rawParamFd_, "Total Pss by OOM adjustment:\n");

    vector<string> reclaimPriority_;
    for (auto reclaim : Memory::ReclaimPriorityMapping) {
        reclaimPriority_.push_back(reclaim.second);
    }
    reclaimPriority_.push_back(Memory::RECLAIM_PRIORITY_UNKNOWN_DESC);

    for (const auto &adjLabel : reclaimPriority_) {
        vector<MemInfoData::MemUsage> memUsages = adjMemResult_[adjLabel];
        vector<string> label;
        if (memUsages.size() == 0) {
            SaveStringToFd(rawParamFd_, adjLabel + ": " + AddKbUnit(0) + "\n");
            continue;
        }
        uint64_t totalPss = accumulate(memUsages.begin(), memUsages.end(), (uint64_t)0,
        [] (uint64_t a, MemInfoData::MemUsage &b) {
            return a + b.pss + b.swapPss;
        });
        SaveStringToFd(rawParamFd_, adjLabel + ": " + AddKbUnit(totalPss) + "\n");

        std::sort(memUsages.begin(), memUsages.end(),
            [] (MemInfoData::MemUsage &left, MemInfoData::MemUsage &right) {
            return right.pss + right.swapPss < left.pss + left.swapPss;
        });
        for (const auto &memUsage : memUsages) {
            string name = PRE_BLANK + memUsage.name + "(pid=" + to_string(memUsage.pid) + "): ";
            StringUtils::GetInstance().SetWidth(NAME_AND_PID_WIDTH, BLANK_, true, name);
            name += AddKbUnit(memUsage.pss + memUsage.swapPss);
            if (memUsage.swapPss > 0) {
                name += " (" + to_string(memUsage.swapPss) + " kB in SwapPss)";
            }
            SaveStringToFd(rawParamFd_, name + "\n");
        }
    }
}
#endif

void MemoryInfo::SetPss(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.pss += value;
}

void MemoryInfo::SetSharedClean(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.sharedClean += value;
}

void MemoryInfo::SetSharedDirty(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.sharedDirty += value;
}

void MemoryInfo::SetPrivateClean(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.privateClean += value;
}

void MemoryInfo::SetPrivateDirty(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.privateDirty += value;
}

void MemoryInfo::SetSwap(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.swap += value;
}

void MemoryInfo::SetSwapPss(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.swapPss += value;
}
void MemoryInfo::SetHeapSize(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.heapSize += value;
}

void MemoryInfo::SetHeapAlloc(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.heapAlloc += value;
}

void MemoryInfo::SetHeapFree(MemInfoData::MemInfo &meminfo, uint64_t value)
{
    meminfo.heapFree += value;
}
} // namespace HiviewDFX
} // namespace OHOS