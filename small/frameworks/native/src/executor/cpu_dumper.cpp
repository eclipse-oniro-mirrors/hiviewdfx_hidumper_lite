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
#include <unistd.h>
#include <dirent.h>
#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include "executor/cpu_dumper.h"
#include "dump_common_utils.h"
#include "dump_utils.h"
#include "hilog_wrapper.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int SAMPLE_INTERVAL_MS = 1000;           // sample interval between two /proc/stat reads
constexpr int SAMPLE_INTERVAL_US = SAMPLE_INTERVAL_MS * 1000;
constexpr int PERCENT_MULT = 100;                  // to percentage value
constexpr int PROC_CPU_LENGTH = 256;               // max length of one output line
constexpr size_t LOAD_AVG_INFO_COUNT = 3;          // load average of 1/5/15 min
constexpr int AVG_INFO_SUBSTR_LENGTH = 4;          // keep 4 chars of load average value
const std::string PROC_STAT_PATH = "/proc/stat";
const std::string PROC_LOADAVG_PATH = "/proc/loadavg";
const std::string CPU_STR = "cpu";
const int TOTAL_CPU_ID = -1;
const int INVALID_PID = -1;

const char *PROC_DETAILS_HEADER = "Details of Processes:";
const char *PROC_TABLE_HEADER =
    "    PID   Total Usage\t   User Space    Kernel Space    Page Fault Minor    Page Fault Major    Name";
const char *PROC_TABLE_ROW =
    "    %-5s    %6.2f%%         %6.2f%%        %6.2f%%        %8s            %8s            %-15s";
const char *TOTAL_CPU_LINE =
    "Total: %.2f%%; User Space: %.2f%%; Kernel Space: %.2f%%; iowait: %.2f%%; irq: %.2f%%; idle: %.2f%%";

struct CpuJiffies {
    uint64_t user = 0;
    uint64_t nice = 0;
    uint64_t system = 0;
    uint64_t idle = 0;
    uint64_t iowait = 0;
    uint64_t irq = 0;
    uint64_t softirq = 0;
    uint64_t steal = 0;

    uint64_t Total() const
    {
        return user + nice + system + idle + iowait + irq + softirq + steal;
    }
};

struct ProcStat {
    int pid = 0;
    std::string comm;
    uint64_t minflt = 0;    uint64_t majflt = 0;    uint64_t utime = 0;    uint64_t stime = 0;};

std::string GetCurrTime()
{
    std::string time;
    DumpCommonUtils::GetDateAndTime(DumpCommonUtils::GetMilliseconds() / 1000, time);
    return time;
}

bool ReadCpuJiffies(std::map<int, CpuJiffies> &jiffies)
{
    std::ifstream file(PROC_STAT_PATH);
    if (!file.is_open()) {
        DUMPER_HILOGE(MODULE_COMMON, "open %{public}s failed", PROC_STAT_PATH.c_str());
        return false;
    }
    std::string line;
    jiffies.clear();
    while (std::getline(file, line)) {
        if (line.compare(0, CPU_STR.size(), CPU_STR) != 0) {
            break;        }
        std::istringstream iss(line);
        std::string cpuName;
        CpuJiffies item;
        iss >> cpuName >> item.user >> item.nice >> item.system >> item.idle >> item.iowait
            >> item.irq >> item.softirq >> item.steal;
        int cpuId = TOTAL_CPU_ID;
        if (cpuName.size() > CPU_STR.size()) { // cpu0 cpu1 ...
            cpuId = std::stoi(cpuName.substr(CPU_STR.size()));
        }
        jiffies[cpuId] = item;
    }
    return !jiffies.empty();
}

bool ReadProcStat(int pid, ProcStat &stat)
{
    std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
    std::ifstream statFile(statPath);
    if (!statFile.is_open()) {
        return false;
    }
    std::string statLine;
    if (!std::getline(statFile, statLine)) {
        return false;
    }
    auto closeBracket = statLine.rfind(')');
    auto openBracket = statLine.find('(');
    if (closeBracket == std::string::npos || openBracket == std::string::npos) {
        return false;
    }
    std::istringstream iss(statLine.substr(closeBracket + 1));
    int fieldNo = 3;    std::string dummy;
    while (fieldNo < 10) {        iss >> dummy;
        fieldNo++;
    }
    stat.pid = pid;
    stat.comm = statLine.substr(openBracket + 1, closeBracket - openBracket - 1);
    iss >> stat.minflt >> stat.majflt >> dummy >> dummy >> dummy >> stat.utime >> stat.stime;
    return true;
}

bool ReadAllProcStats(std::map<int, ProcStat> &stats)
{
    stats.clear();
    std::vector<int32_t> pids = DumpCommonUtils::GetAllPids();
    for (auto pid : pids) {
        ProcStat stat;
        if (ReadProcStat(pid, stat)) {
            stats[pid] = stat;
        }
    }
    return !stats.empty();
}

std::string ReadLoadAvg()
{
    std::ifstream file(PROC_LOADAVG_PATH);
    if (!file.is_open()) {
        DUMPER_HILOGE(MODULE_COMMON, "open %{public}s failed", PROC_LOADAVG_PATH.c_str());
        return "";
    }
    double loads[LOAD_AVG_INFO_COUNT] = {0};
    file >> loads[0] >> loads[1] >> loads[2];
    std::string info = "Load average:";
    for (size_t i = 0; i < LOAD_AVG_INFO_COUNT; i++) {
        info.append(" ");
        info.append(std::to_string(loads[i]).substr(0, AVG_INFO_SUBSTR_LENGTH));
        if (i == LOAD_AVG_INFO_COUNT - 1) {
            info.append(";");
        } else {
            info.append(" /");
        }
    }
    info.append(" the cpu load average in 1 min, 5 min and 15 min");
    return info;
}

bool SortProcUsage(const ProcStat &left, double leftTotal, double leftUser, double leftSys,
                   const ProcStat &right, double rightTotal, double rightUser, double rightSys)
{
    if (rightTotal != leftTotal) {
        return rightTotal < leftTotal;
    }
    if (rightUser != leftUser) {
        return rightUser < leftUser;
    }
    if (rightSys != leftSys) {
        return rightSys < leftSys;
    }
    std::string leftPid = std::to_string(left.pid);
    std::string rightPid = std::to_string(right.pid);
    if (rightPid.length() != leftPid.length()) {
        return rightPid.length() < leftPid.length();
    }
    return rightPid.compare(leftPid) < 0;
}
} // namespace

CPUDumper::CPUDumper()
{
}

CPUDumper::~CPUDumper()
{
}

DumpStatus CPUDumper::PreExecute(const std::shared_ptr<DumperParameter> &parameter, StringMatrix dumpDatas)
{
    DUMPER_HILOGI(MODULE_COMMON, "info|CPUDumper PreExecute");
    dumpCPUDatas_ = dumpDatas;
    isDumpCpuUsage_ = (parameter->GetOpts()).isDumpCpuUsage_;
    cpuUsagePid_ = (parameter->GetOpts()).cpuUsagePid_;
    return DumpStatus::DUMP_OK;
}

DumpStatus CPUDumper::Execute()
{
    DUMPER_HILOGI(MODULE_COMMON, "info|CPUDumper Execute");
    if (!isDumpCpuUsage_) {
        return DumpStatus::DUMP_FAIL;
    }
    startTime_ = GetCurrTime();
    dumpCPUDatas_->clear();
    bool ret = DumpCpuUsage(cpuUsagePid_);
    DUMPER_HILOGI(MODULE_COMMON, "info|CPUDumper Execute end, ret=%{public}d", ret);
    return ret ? DumpStatus::DUMP_OK : DumpStatus::DUMP_FAIL;
}

DumpStatus CPUDumper::AfterExecute()
{
    return DumpStatus::DUMP_OK;
}

bool CPUDumper::DumpCpuUsage(int pid)
{
    std::map<int, CpuJiffies> cpuFirst;
    std::map<int, CpuJiffies> cpuSecond;
    std::map<int, ProcStat> procFirst;
    std::map<int, ProcStat> procSecond;
    if (!ReadCpuJiffies(cpuFirst) || !ReadAllProcStats(procFirst)) {
        return false;
    }
    usleep(SAMPLE_INTERVAL_US);
    if (!ReadCpuJiffies(cpuSecond)) {
        return false;
    }
    if (pid > 0) {
        ProcStat stat;
        if (!ReadProcStat(pid, stat)) {
            return false;
        }
        procSecond[pid] = stat;
    } else if (!ReadAllProcStats(procSecond)) {
        return false;
    }
    endTime_ = GetCurrTime();
    const CpuJiffies &prev = cpuFirst[TOTAL_CPU_ID];
    const CpuJiffies &cur = cpuSecond[TOTAL_CPU_ID];
    uint64_t total = cur.Total() - prev.Total();
    if (total == 0) {
        return false;
    }

    std::vector<std::string> loadLine;
    loadLine.push_back(ReadLoadAvg());
    dumpCPUDatas_->push_back(loadLine);

    std::vector<std::string> timeLine;
    timeLine.push_back("CPU usage from " + startTime_ + " to " + endTime_);
    dumpCPUDatas_->push_back(timeLine);

    double userSpace = PERCENT_MULT * static_cast<double>(cur.user + cur.nice - prev.user - prev.nice) / total;
    double sysSpace = PERCENT_MULT * static_cast<double>(cur.system - prev.system) / total;
    double ioWait = PERCENT_MULT * static_cast<double>(cur.iowait - prev.iowait) / total;
    double irq = PERCENT_MULT * static_cast<double>(cur.irq + cur.softirq - prev.irq - prev.softirq) / total;
    double idle = PERCENT_MULT * static_cast<double>(cur.idle - prev.idle) / total;
    char totalLine[PROC_CPU_LENGTH] = {0};
    if (sprintf_s(totalLine, sizeof(totalLine), TOTAL_CPU_LINE,
        userSpace + sysSpace, userSpace, sysSpace, ioWait, irq, idle) < 0) {
        return false;
    }
    std::vector<std::string> totalVec;
    totalVec.push_back(totalLine);
    dumpCPUDatas_->push_back(totalVec);

    struct ProcUsage {
        ProcStat stat;
        double totalUsage;
        double userUsage;
        double sysUsage;
    };
    std::vector<ProcUsage> usages;
    for (const auto &item : procSecond) {
        auto it = procFirst.find(item.first);
        if (it == procFirst.end()) {
            continue;        }
        ProcUsage usage;
        usage.stat = item.second;
        usage.userUsage = PERCENT_MULT * static_cast<double>(item.second.utime - it->second.utime) / total;
        usage.sysUsage = PERCENT_MULT * static_cast<double>(item.second.stime - it->second.stime) / total;
        usage.totalUsage = usage.userUsage + usage.sysUsage;
        usages.push_back(usage);
    }
    std::sort(usages.begin(), usages.end(), [](const ProcUsage &left, const ProcUsage &right) {
        return SortProcUsage(left.stat, left.totalUsage, left.userUsage, left.sysUsage,
                             right.stat, right.totalUsage, right.userUsage, right.sysUsage);
    });

    std::vector<std::string> detailLine;
    detailLine.push_back(PROC_DETAILS_HEADER);
    dumpCPUDatas_->push_back(detailLine);
    std::vector<std::string> tableLine;
    tableLine.push_back(PROC_TABLE_HEADER);
    dumpCPUDatas_->push_back(tableLine);
    for (const auto &usage : usages) {
        char row[PROC_CPU_LENGTH] = {0};
        if (sprintf_s(row, sizeof(row), PROC_TABLE_ROW, std::to_string(usage.stat.pid).c_str(),
            usage.totalUsage, usage.userUsage, usage.sysUsage,
            std::to_string(usage.stat.minflt).c_str(), std::to_string(usage.stat.majflt).c_str(),
            usage.stat.comm.c_str()) < 0) {
            continue;
        }
        std::vector<std::string> rowVec;
        rowVec.push_back(row);
        dumpCPUDatas_->push_back(rowVec);
    }
    return true;
}

DumpCpuData CPUDumper::GetDumpCpuData()
{
    DumpCpuData dumpCpu;
    dumpCpu.startTime_ = startTime_;
    dumpCpu.endTime_ = endTime_;
    dumpCpu.dumpCPUDatas_ = *dumpCPUDatas_;
    dumpCpu.cpuUsagePid_ = cpuUsagePid_;
    return dumpCpu;
}
} // namespace HiviewDFX
} // namespace OHOS
