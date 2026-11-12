/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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
#include "manager/dump_implement.h"
#include <unistd.h>
#include "executor/fd_output.h"
#include "hilog_wrapper.h"
#include "util/config_utils.h"
#include "factory/cpu_dumper_factory.h"
#include "factory/cmd_dumper_factory.h"
#include "factory/dumper_group_factory.h"
#include "factory/file_dumper_factory.h"
#include "factory/memory_dumper_factory.h"
#include "dump_utils.h"
#include "string_ex.h"
#include "file_ex.h"
#include "util/string_utils.h"
#include "common/dumper_constant.h"
#include "securec.h"

namespace OHOS {
namespace HiviewDFX {
static struct option g_longOptions[] = {{"cpufreq", no_argument, nullptr, 0},
    {"cpuusage", optional_argument, nullptr, 0},
    {"mem", optional_argument, nullptr, 0},
    {"prune", no_argument, nullptr, 0},
    {nullptr, 0, nullptr, 0}};

DumpImplement::DumpImplement()
{
    AddExecutorFactoryToMap();
}

DumpImplement::~DumpImplement()
{
}

void DumpImplement::AddExecutorFactoryToMap()
{
    ptrExecutorFactoryMap_ = std::make_shared<ExecutorFactoryMap>();
    ptrExecutorFactoryMap_->insert(std::make_pair(DumperConstant::CPU_DUMPER, std::make_shared<CPUDumperFactory>()));
    ptrExecutorFactoryMap_->insert(std::make_pair(DumperConstant::FILE_DUMPER, std::make_shared<FileDumperFactory>()));
    ptrExecutorFactoryMap_->insert(std::make_pair(DumperConstant::CMD_DUMPER, std::make_shared<CMDDumperFactory>()));
    ptrExecutorFactoryMap_->insert(std::make_pair(DumperConstant::GROUP, std::make_shared<DumperGroupFactory>()));
    ptrExecutorFactoryMap_->insert(
        std::make_pair(DumperConstant::MEMORY_DUMPER, std::make_shared<MemoryDumperFactory>()));
}

DumpStatus DumpImplement::Main(int argc, char *argv[], const std::shared_ptr<RawParam> &reqCtl)
{
    std::shared_ptr<DumperParameter> ptrDumperParameter = std::make_shared<DumperParameter>();
    DumpStatus ret = InitHandle(argc, argv, reqCtl, ptrDumperParameter);
    if (ret != DumpStatus::DUMP_OK) {
        return ret;
    }

    const DumperOpts &opts = ptrDumperParameter->GetOpts();
    DumpStatus finalRet = DumpStatus::DUMP_OK;
    if (opts.isDumpCpuFreq_ || opts.isDumpCpuUsage_ || opts.isDumpMem_ || opts.isDumpProcesses_) {
        std::vector<std::shared_ptr<DumpCfg>> &configs = ptrDumperParameter->GetExecutorConfigList();
        DUMPER_HILOGD(MODULE_COMMON, "debug|Main configs size is %{public}zu", configs.size());
        if (configs.size() == 0) {
            DUMPER_HILOGE(MODULE_COMMON, "Executor config list is empty, so can not dump.");
            return DumpStatus::DUMP_FAIL;
        }
        std::vector<std::shared_ptr<HidumperExecutor>> hidumperExecutors;
        setExecutorList(hidumperExecutors, configs);
        if (hidumperExecutors.empty()) {
            DUMPER_HILOGE(MODULE_COMMON, "Executor list is empty, so dump fail.");
            return DumpStatus::DUMP_FAIL;
        }

        HidumperExecutor::StringMatrix dumpDatas = std::make_shared<std::vector<std::vector<std::string>>>();
        DumpStatus oldRet = DumpDatas(hidumperExecutors, ptrDumperParameter, dumpDatas);
        if (oldRet != DumpStatus::DUMP_OK) {
            DUMPER_HILOGE(MODULE_COMMON, "DUMP FAIL!!!");
            finalRet = oldRet;
        }
    }
    return finalRet;
}

DumpStatus DumpImplement::InitHandle(int argc, char *argv[], const std::shared_ptr<RawParam> &reqCtl,
    std::shared_ptr<DumperParameter>& ptrDumperParameter)
{
    ptrDumperParameter->setClientCallback(reqCtl);
    ptrDumperParameter->SetPid(reqCtl->GetPid());
    ptrDumperParameter->SetUid(reqCtl->GetUid());
    std::lock_guard<std::mutex> lock(mutexCmdLock_); // lock for optind value safe
    DumpStatus ret = CmdParse(argc, argv, ptrDumperParameter);
    if (ret != DumpStatus::DUMP_OK) {
        DUMPER_HILOGE(MODULE_COMMON, "Parse cmd FAIL!!!");
        return ret;
    }
    ConfigUtils::GetDumperConfigs(ptrDumperParameter);
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::CheckArgs(int argc, char* argv[])
{
    if (argc > ARG_MAX_COUNT) {
        DUMPER_HILOGE(MODULE_COMMON, "too many arguments(%{public}d), limit size %{public}d.", argc, ARG_MAX_COUNT);
        return DumpStatus::DUMP_FAIL;
    }
    for (int i = 0; i < argc; i++) {
        if (argv[i] == nullptr) {
            DUMPER_HILOGE(MODULE_COMMON, "argument(%{public}d) is null.", i);
            return DumpStatus::DUMP_FAIL;
        }
        size_t len = strlen(argv[i]);
        if (len == 0) {
            DUMPER_HILOGE(MODULE_COMMON, "argument(%{public}d) is empty.", i);
            return DumpStatus::DUMP_FAIL;
        }
        if (len > SINGLE_ARG_MAXLEN) {
            DUMPER_HILOGE(MODULE_COMMON, "too long args:%{public}zu, limit size:%{public}d.", len, SINGLE_ARG_MAXLEN);
            return DumpStatus::DUMP_FAIL;
        }
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::CmdParse(int argc, char *argv[], std::shared_ptr<DumperParameter> &dumpParameter)
{
    if (CheckArgs(argc, argv) != DumpStatus::DUMP_OK)
        return DumpStatus::DUMP_FAIL;
    DumperOpts opts;
    DumpStatus status = CmdParseWithParameter(dumpParameter, argc, argv, opts);
    if (status != DumpStatus::DUMP_OK)
        return status;
    if (!opts.IsSelectAny()) {
        ptrReqCtl_ = dumpParameter->getClientCallback();
        CmdHelp();
        ptrReqCtl_ = nullptr;
        return DumpStatus::DUMP_HELP;
    }
    dumpParameter->SetOpts(opts);
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::CmdParseWithParameter(int argc, char *argv[], DumperOpts &opts)
{
    optind = 0; // reset getopt_long
    opterr = 0; // getopt not show error info
    const char optStr[] = "-hpt:";
    int optionIndex = 0;
    int c = getopt_long(argc, argv, optStr, g_longOptions, &optionIndex);
    while (c != -1) {
        DUMPER_HILOGD(MODULE_COMMON, "debug|getopt_long(), c=%{public}d, optind=%{public}d", c, optind);
        if (c == 0) {
            DumpStatus status = ParseLongCmdOption(opts, g_longOptions, optionIndex);
            if (status != DumpStatus::DUMP_OK) {
                return status;
            }
        } else if (c == 'h') {
            CmdHelp();
            return DumpStatus::DUMP_HELP;
        } else if (c == '?') {
            CheckIncorrectCmdOption(optStr, argv);
            return DumpStatus::DUMP_INVALID_ARG;
        } else {
            DumpStatus status = ParseShortCmdOption(c, opts, argc, argv);
            if (status != DumpStatus::DUMP_OK) {
                return status;
            }
        }
        optionIndex = 0;
        c = getopt_long(argc, argv, optStr, g_longOptions, &optionIndex);
    }
    DumpStatus status = CheckProcessAlive(opts);
    if (status != DumpStatus::DUMP_OK) {
        return status;
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::CmdParseWithParameter(std::shared_ptr<DumperParameter> &dumpParameter, int argc, char *argv[],
                                                DumperOpts &opts)
{
    DUMPER_HILOGD(MODULE_COMMON, "enter|");
    ptrReqCtl_ = dumpParameter->getClientCallback();
    DumpStatus ret = CmdParseWithParameter(argc, argv, opts);
    if (ret == DumpStatus::DUMP_OK) {
        std::string errorStr;
        if (!opts.CheckOptions(errorStr)) {
            SendErrorMessage(invalidError_ + errorStr);
            ret = DumpStatus::DUMP_INVALID_ARG;
        }
    }
    ptrReqCtl_ = nullptr;
    DUMPER_HILOGD(MODULE_COMMON, "leave|ret=%{public}d", ret);
    return ret;
}

DumpStatus DumpImplement::SetCmdParameter(int argc, char *argv[], DumperOpts &opts)
{
    DumpStatus status = DumpStatus::DUMP_OK;
    DUMPER_HILOGD(MODULE_COMMON,
                  "debug|SetCmdParameter optind is %{public}d"
                  " argc is  %{public}d",
                  optind,
                  argc);
    if (optind > 1 && optind <= argc) {
        std::string optionName = argv[optind - ARG_INDEX_OFFSET_LAST_OPTION];
        std::string optionValue = argv[optind - 1];
        status = HandleOptionParameter(optionName, optionValue, opts);
        if (status == DumpStatus::DUMP_FAIL) {
            return status;
        }
    }
    return status;
}

DumpStatus DumpImplement::HandleOptionParameter(const std::string &optionName,
                                                const std::string &optionValue, DumperOpts &opts)
{
    DumpStatus status = DumpStatus::DUMP_OK;
    if (optionName == "--cpuusage") {
        status = SetCmdIntegerParameter(optionValue, opts.cpuUsagePid_);
    } else if (optionName == "--mem") {
        if (optionValue == "SIGINT") {
            opts.isReceivedSigInt_ = true;
        } else {
            status = SetCmdIntegerParameter(optionValue, opts.memPid_);
        }
    } else if (optionName == "-p") {
        status = SetCmdIntegerParameter(optionValue, opts.processPid_);
    } else {
        std::string errorStr = unrecognizedError_ + RemoveCharacterFromStr(optionValue, '-');
        SendErrorMessage(errorStr);
        return DumpStatus::DUMP_FAIL;
    }
    return status;
}

bool DumpImplement::ParseSubLongCmdOption(DumperOpts &opts, const struct option longOptions[],
                                          const int &optionIndex)
{
    if (StringUtils::GetInstance().IsSameStr(longOptions[optionIndex].name, "cpufreq")) {
        opts.isDumpCpuFreq_ = true;
    } else if (StringUtils::GetInstance().IsSameStr(longOptions[optionIndex].name, "cpuusage")) {
        opts.isDumpCpuUsage_ = true;
    } else if (StringUtils::GetInstance().IsSameStr(longOptions[optionIndex].name, "mem")) {
        opts.isDumpMem_ = true;
    } else {
        return false;
    }

    return true;
}

DumpStatus DumpImplement::ParseLongCmdOption(DumperOpts &opts, const struct option longOptions[],
                                             const int &optionIndex)
{
    if (ParseSubLongCmdOption(opts, longOptions, optionIndex)) {
        return DumpStatus::DUMP_OK;
    } else if (StringUtils::GetInstance().IsSameStr(longOptions[optionIndex].name, "prune")) {
        return SetMemPruneParam(opts);
    } else {
        DUMPER_HILOGE(MODULE_COMMON, "ParseLongCmdOption %{public}s", longOptions[optionIndex].name);
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::SetMemPruneParam(DumperOpts &opts)
{
    DumpStatus status = DumpStatus::DUMP_FAIL;
    if (opts.isDumpMem_) {
        opts.dumpMemPrune_ = true;
        status = DumpStatus::DUMP_OK;
    }
    return status;
}

DumpStatus DumpImplement::ParseShortCmdOption(int c, DumperOpts &opts, int argc, char *argv[])
{
    switch (c) {
        case 'p':
            opts.isDumpProcesses_ = true;
            break;
        case 't':
            return SetCmdIntegerParameter(optarg, opts.timeInterval_);
        default: {
            DumpStatus status = SetCmdParameter(argc, argv, opts);
            if (status != DumpStatus::DUMP_OK) {
                return status;
            }
            break;
        }
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus DumpImplement::SetCmdIntegerParameter(const std::string &str, int &value)
{
    if (!IsNumericStr(str)) {
        DUMPER_HILOGE(MODULE_COMMON, "Invalid string arg %{public}s", str.c_str());
        std::string errorStr = invalidError_ + str;
        SendErrorMessage(errorStr);
        return DumpStatus::DUMP_INVALID_ARG;
    }
    if (!StrToInt(str, value)) {
        DUMPER_HILOGE(MODULE_COMMON, "StrToInt error, str=%{public}s", str.c_str());
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}

void DumpImplement::PrintCommonUsage(const std::string& str)
{
    if (ptrReqCtl_ == nullptr) {
        return;
    }
    int rawParamFd = ptrReqCtl_->GetOutputFd();
    if (rawParamFd < 0) {
        return;
    }
    SaveStringToFd(rawParamFd, str.c_str());
}

void DumpImplement::CmdHelp()
{
    const std::string commonUsageStr =
        "usage:\n"
        "  -h                             |help text for the tool\n"
        "  -p                             |dump all processes information\n"
        "  -p [pid]                       |dump information of specified process and its threads\n"
        "  --cpuusage                     |dump system cpu usage of each core\n"
        "  --cpuusage [pid]               |dump cpu usage of threads under specified pid\n"
        "  --cpufreq                      |dump real CPU frequency of each core\n"
        "  --mem                          |dump memory usage of total\n"
        "  --mem [pid]                    |dump memory usage of specified pid\n"
        "  --mem --prune                  |dump simplified memory information\n"
        "  --mem [pid] -t [timeInterval]  |dump process memory change information, press Ctrl+C to stop the export."
        " detail information is stored in /data/log/hidumper/record_mem.txt.\n";

    PrintCommonUsage(commonUsageStr);
}

void DumpImplement::setExecutorList(std::vector<std::shared_ptr<HidumperExecutor>> &executors,
                                    const std::vector<std::shared_ptr<DumpCfg>> &configs)
{
    std::shared_ptr<HidumperExecutor> ptrOutput;

    for (size_t i = 0; i < configs.size(); i++) {
        std::shared_ptr<HidumperExecutor> ptrExecutor;
        if ((configs[i]->class_) == DumperConstant::FD_OUTPUT) {
            if (ptrOutput.get() == nullptr) {
                ptrOutput = std::make_shared<FDOutput>();
            }
            ptrOutput->SetDumpConfig(configs[i]);
            executors.push_back(ptrOutput);
            continue;
        } else {
            ExecutorFactoryMap::iterator it = ptrExecutorFactoryMap_->find(configs[i]->class_);
            if (it != ptrExecutorFactoryMap_->end()) {
                ptrExecutor = it->second->CreateExecutor();
            }
        }

        if (ptrExecutor.get() == nullptr) {
            DUMPER_HILOGE(MODULE_COMMON, "configs[%{public}zu].class_ is %{public}d", i, configs[i]->class_);
            continue;
        }
        configs[i]->executor_ = ptrExecutor;
        ptrExecutor->SetDumpConfig(configs[i]);
        executors.push_back(ptrExecutor);
    }

    // must clear.
    for (auto cfg : configs) {
        cfg->executor_ = nullptr;
    }
}

DumpStatus DumpImplement::DumpDatas(const std::vector<std::shared_ptr<HidumperExecutor>> &executors,
                                    const std::shared_ptr<DumperParameter> &dumpParameter,
                                    HidumperExecutor::StringMatrix dumpDatas)
{
    auto callback = dumpParameter->getClientCallback();

    std::string groupName = "";
    std::vector<size_t> loopStack;
    const size_t executorSum = executors.size();
    for (size_t index = 0; index < executorSum; index++) {
        callback->UpdateProgress(executors.size(), index);
        if (callback->IsCanceled()) {
            break;
        }

        auto dumpCfg = executors[index]->GetDumpConfig();
        if (dumpCfg->IsDumper() && CheckGroupName(groupName, dumpCfg->section_)) {
            AddGroupTitle(groupName, dumpDatas, dumpParameter);
        }

        DumpStatus ret = DumpStatus::DUMP_FAIL;
        ret = executors[index]->DoPreExecute(dumpParameter, dumpDatas);
        if (ret != DumpStatus::DUMP_OK) {
            continue;
        }

        ret = executors[index]->DoExecute();
        if ((ret != DumpStatus::DUMP_OK) && (ret != DumpStatus::DUMP_MORE_DATA)) {
            continue;
        }

        ret = executors[index]->DoAfterExecute();
        if (dumpCfg->IsDumper() && dumpCfg->CanLoop() && (ret == DumpStatus::DUMP_MORE_DATA)) {
            loopStack.push_back(index);
        }

        if (dumpCfg->IsOutput() || dumpCfg->IsGroup()) {
            if (!loopStack.empty()) {
                index = loopStack.back() - 1; // the 1 will add back by end for.
            }
            loopStack.clear(); // clear now.
        }
    }
    for (auto executor : executors) {
        executor->Reset();
    }
    callback->UpdateProgress(executors.size(), executors.size());
    return DumpStatus::DUMP_OK;
}

void DumpImplement::AddGroupTitle(const std::string &groupName, HidumperExecutor::StringMatrix dumpDatas,
    const std::shared_ptr<DumperParameter>& dumpParameter)
{
    /**
     * @brief The group title is followed
     * '
     * -------------------------------[groupName]-------------------------------
     * '
     */
    if (StringUtils::GetInstance().IsSameStr(groupName, "memory") && dumpParameter->GetOpts().memPid_ <= 0) {
        DUMPER_HILOGI(MODULE_COMMON, "hidumper --mem cmd, do not need title.");
        return;
    }
    if (StringUtils::GetInstance().IsSameStr(groupName, "memory") && dumpParameter->GetOpts().timeInterval_ > 0) {
        DUMPER_HILOGI(MODULE_COMMON, "hidumper --mem pid -t timeinterval cmd, do not need title.");
        return;
    }
    std::vector<std::string> lineData;
    lineData.push_back("");
    dumpDatas->push_back(lineData);
    std::vector<std::string>().swap(lineData);
    lineData.push_back("-------------------------------[");
    lineData.push_back(groupName);
    lineData.push_back("]-------------------------------");
    dumpDatas->push_back(lineData);
    std::vector<std::string>().swap(lineData);
    lineData.push_back("");
    dumpDatas->push_back(lineData);
    std::vector<std::string>().swap(lineData);
}

bool DumpImplement::CheckGroupName(std::string &lastName, const std::string &curName)
{
    if (curName.compare("") == 0) {
        return false;
    }

    if (lastName.compare(curName) == 0) {
        return false;
    }

    lastName.assign(curName);
    return true;
}

void DumpImplement::CheckIncorrectCmdOption(const char *optStr, char *argv[])
{
    if (optopt == 0) {
        std::string optLongStr = RemoveCharacterFromStr(argv[optind - 1], '-');
        SendErrorMessage(unrecognizedError_ + optLongStr);
    } else if (!IsShortOptionReqArg(optStr)) {
        std::string errorStr = unrecognizedError_;
        errorStr += optopt;
        SendErrorMessage(errorStr);
    }
}

bool DumpImplement::IsShortOptionReqArg(const char *optStr)
{
    int len = strlen(optStr);
    for (int i = 0; i < len; i++) {
        if (optStr[i] == optopt) {
            SendErrorMessage(requireError_ + optStr[i]);
            return true;
        }
    }
    return false;
}

void DumpImplement::SendErrorMessage(const std::string &errorStr)
{
    if (ptrReqCtl_ == nullptr) {
        return;
    }
    int rawParamFd = ptrReqCtl_->GetOutputFd();
    if (rawParamFd < 0) {
        return;
    }
    SaveStringToFd(rawParamFd, errorStr + "\n");
}

void DumpImplement::SendPidErrorMessage(int pid)
{
    if (ptrReqCtl_ == nullptr) {
        return;
    }
    int rawParamFd = ptrReqCtl_->GetOutputFd();
    if (rawParamFd < 0) {
        return;
    }
    dprintf(rawParamFd, pidError_.c_str(), pid);
}

std::string DumpImplement::RemoveCharacterFromStr(const std::string &str, const char character)
{
    std::string strTmp = str;
    while (strTmp.find(character) != std::string::npos) {
        strTmp.erase(strTmp.find(character), 1);
    }
    return strTmp;
}

DumpStatus DumpImplement::CheckProcessAlive(const DumperOpts &opts)
{
    if ((opts.cpuUsagePid_ > -1) && !DumpUtils::CheckProcessAlive(opts.cpuUsagePid_)) {
        SendPidErrorMessage(opts.cpuUsagePid_);
        return DumpStatus::DUMP_FAIL;
    }
    if ((opts.memPid_ > -1) && !DumpUtils::CheckProcessAlive(opts.memPid_)) {
        SendPidErrorMessage(opts.memPid_);
        return DumpStatus::DUMP_FAIL;
    }
    if ((opts.processPid_ > -1) && !DumpUtils::CheckProcessAlive(opts.processPid_)) {
        SendPidErrorMessage(opts.processPid_);
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}
} // namespace HiviewDFX
} // namespace OHOS
