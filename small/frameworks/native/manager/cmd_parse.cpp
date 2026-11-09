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

#include "cmd_parse.h"

#include "common.h"
#include "dump_context.h"
#include "dump_utils.h"
#include "hilog_wrapper.h"
#include "file_ex.h"
#include "string_ex.h"
#include "securec.h"
#include "unistd.h"
#include "writer_utils.h"

namespace OHOS {
namespace HiviewDFX {
namespace {
constexpr int ARG_MAX_COUNT = 100;
constexpr int SINGLE_ARG_MAXLEN = 1024;
constexpr int ARG_INDEX_OFFSET_LAST_OPTION = 2;
const std::string UNRECOGNIZED_ERROR_MSG = "hidumper: option pid missed. ";
const std::string ARGUMENT_ERROR_MSG = "hidumper: option requires an argument: ";
const std::string INVALID_ARG_MSG = "hidumper: invalid arg: ";
const std::string PID_ERROR_MSG = "hidumper: No such process: ";

static struct option LONG_OPTIONS[] = {
    {"cpufreq", no_argument, 0, 0},
    {"cpuusage", optional_argument, 0, 0},
    {"mem", optional_argument, 0, 0},
    {"prune", no_argument, 0, 0},
    {0, 0, 0, 0}
};

void PrintHelpInfo(DumpContext& dumpContext)
{
    const std::string helpInfo =
        "usage:\n"
        "  -h                          |help text for the tool\n"
        "  -p                          |dump all processes information\n"
        "  -p [pid]                    |dump information of specified process and its threads\n"
        "  --cpuusage                  |dump system cpu usage of each core\n"
        "  --cpuusage [pid]            |dump cpu usage of threads under specified pid\n"
        "  --cpufreq                   |dump real CPU frequency of each core\n"
        "  --mem                       |dump memory usage of total\n"
        "  --mem [pid]                 |dump memory usage of specified pid\n"
        "  --mem --prune               |dump simplified memory information\n"
        "  --mem [pid] -t [timeInterval]  |dump process memory change information, press Ctrl+C to stop the export."
        " detail information is stored in /data/log/hidumper/record_mem.txt.\n";
    WriteStringIntoFd(helpInfo, dumpContext.GetOutputFd());
}

void PrintErrorMessage(const DumpContext& dumpContext, const std::string& errorMsg)
{
    WriteStringIntoFd(errorMsg, dumpContext.GetOutputFd());
}

bool IsArgValid(char* arg)
{
    if (arg == nullptr) {
        DUMPER_HILOGE(MODULE_COMMON, "argument is null.");
        return false;
    }
    size_t len = strlen(arg);
    if (len == 0) {
        DUMPER_HILOGE(MODULE_COMMON, "argument is empty.");
        return false;
    }
    if (len > SINGLE_ARG_MAXLEN) {
        DUMPER_HILOGE(MODULE_COMMON, "too long args:%{public}zu, limit size:%{public}d.", len, SINGLE_ARG_MAXLEN);
        return false;
    }
    return true;
}
}

CmdParse::CmdParse() {}
CmdParse::~CmdParse() {}

DumpStatus CmdParse::Parse(int argc, char *argv[], DumpContext& dumpContext)
{
    if (CheckArgs(argc, argv) != DumpStatus::DUMP_OK) {
        DUMPER_HILOGE(MODULE_COMMON, "Invalid input parameters");
        return DumpStatus::DUMP_FAIL;
    }
    if (CmdParseHelper(argc, argv, dumpContext) != DumpStatus::DUMP_OK) {
        DUMPER_HILOGE(MODULE_COMMON, "CmdParseHelper failed");
        return DumpStatus::DUMP_FAIL;
    }
    if (VerifyOptions(dumpContext) != DumpStatus::DUMP_OK) {
        DUMPER_HILOGE(MODULE_COMMON, "VerifyOptions failed");
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus CmdParse::CheckArgs(int argc, char* argv[])
{
    if (argc < 1 || argv == nullptr) {
        DUMPER_HILOGE(MODULE_COMMON, "Invalid input parameters");
        return DumpStatus::DUMP_FAIL;
    }
    if (argc > ARG_MAX_COUNT) {
        DUMPER_HILOGE(MODULE_COMMON, "too many arguments(%{public}d), limit size %{public}d.", argc, ARG_MAX_COUNT);
        return DumpStatus::DUMP_FAIL;
    }
    for (int i = 0; i < argc; i++) {
        if (!IsArgValid(argv[i])) {
            return DumpStatus::DUMP_FAIL;
        }
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus CmdParse::CmdParseHelper(int argc, char *argv[], DumpContext& dumpContext)
{
    optind = 0; // reset getopt_long
    opterr = 0; // getopt not show error info
    const char optStr[] = "-hpt:";

    while (true) {
        int optionIndex = 0;
        int c = getopt_long(argc, argv, optStr, LONG_OPTIONS, &optionIndex);
        if (c == -1) {
            break;
        }

        DumpStatus status;
        switch (c) {
            case 0:
                status = ParseLongCmdOption(dumpContext, LONG_OPTIONS, optionIndex);
                break;
            case 'h':
                PrintHelpInfo(dumpContext);
                return DumpStatus::DUMP_HELP;
            case '?':
                CheckIncorrectCmdOption(optStr, argv, dumpContext);
                return DumpStatus::DUMP_INVALID_ARG;
            default:
                status = ParseShortCmdOption(c, dumpContext, argc, argv);
                break;
        }
        if (status != DumpStatus::DUMP_OK) {
            return status;
        }
    }
    if (CheckProcessAlive(dumpContext) != DumpStatus::DUMP_OK) {
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus CmdParse::ParseLongCmdOption(DumpContext& dumpContext,
                                        const struct option longOptions[], const int &optionIndex)
{
    if (ParseSubLongCmdOption(dumpContext, longOptions, optionIndex)) {
        return DumpStatus::DUMP_OK;
    }

    const std::string optionName = longOptions[optionIndex].name;
    if (optionName == "prune") {
        return DumpStatus::DUMP_OK;
    }
    DUMPER_HILOGE(MODULE_COMMON, "ParseLongCmdOption %{public}s", optionName.c_str());
    return DumpStatus::DUMP_OK;
}

bool CmdParse::ParseSubLongCmdOption(DumpContext& dumpContext,
                                     const struct option longOptions[], const int &optionIndex)
{
    const std::string optionName = longOptions[optionIndex].name;
    if (optionName == "cpufreq") {
        dumpContext.GetDumperOpts()->isDumpCpuFreq = true;
    } else if (optionName == "cpuusage") {
        dumpContext.GetDumperOpts()->isDumpCpuUsage = true;
    } else if (optionName == "mem") {
        dumpContext.GetDumperOpts()->isDumpMem = true;
    } else {
        return false;
    }

    return true;
}

void CmdParse::CheckIncorrectCmdOption(const char *optStr, char *argv[], DumpContext& dumpContext)
{
    if (optopt == 0) {
        std::string optionName = RemoveCharacterFromStr(argv[optind - 1], '-');
        PrintErrorMessage(dumpContext, UNRECOGNIZED_ERROR_MSG + optionName);
    } else if (!IsShortOptionReqArg(optStr, dumpContext)) {
        std::string errorStr = UNRECOGNIZED_ERROR_MSG;
        errorStr += optopt;
        PrintErrorMessage(dumpContext, errorStr);
    }
}

DumpStatus CmdParse::ParseShortCmdOption(int c, DumpContext& dumpContext, int argc, char *argv[])
{
    switch (c) {
        case 'p':
            dumpContext.GetDumperOpts()->isDumpProcesses = true;
            break;
        case 't':
            break;
        default:
            return SetCmdParameter(argc, argv, dumpContext);
    }
    return DumpStatus::DUMP_OK;
}

bool CmdParse::IsShortOptionReqArg(const char *optStr, DumpContext& dumpContext)
{
    for (int i = 0; optStr[i] != '\0'; ++i) {
        if (optStr[i] == optopt) {
            std::string errorMsg = ARGUMENT_ERROR_MSG + std::string(1, optStr[i]);
            PrintErrorMessage(dumpContext, errorMsg);
            return true;
        }
    }
    return false;
}

DumpStatus CmdParse::SetCmdParameter(int argc, char *argv[], DumpContext& dumpContext)
{
    DUMPER_HILOGD(MODULE_COMMON,
                  "debug|SetCmdParameter optind is %{public}d"
                  " argc is  %{public}d",
                  optind,
                  argc);
    if (optind > 1 && optind <= argc) {
        const std::string prevArg = argv[optind - ARG_INDEX_OFFSET_LAST_OPTION];
        const std::string currentArg = argv[optind - 1];

        if (prevArg == "--cpuusage") {
            return SetCmdIntegerParameter(currentArg, dumpContext.GetDumperOpts()->cpuUsagePid, dumpContext);
        } else if (prevArg == "--mem") {
            return SetCmdIntegerParameter(currentArg, dumpContext.GetDumperOpts()->memPid, dumpContext);
        } else if (prevArg == "-p") {
            return SetCmdIntegerParameter(currentArg, dumpContext.GetDumperOpts()->processPid, dumpContext);
        } else {
            std::string optionName = RemoveCharacterFromStr(currentArg, '-');
            std::string errorStr = UNRECOGNIZED_ERROR_MSG + optionName;
            PrintErrorMessage(dumpContext, errorStr);
            return DumpStatus::DUMP_FAIL;
        }
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus CmdParse::SetCmdIntegerParameter(const std::string &str, int &value, DumpContext& dumpContext)
{
    if (!IsNumericStr(str)) {
        DUMPER_HILOGE(MODULE_COMMON, "Invalid string arg %{public}s", str.c_str());
        PrintErrorMessage(dumpContext, INVALID_ARG_MSG + str);
        return DumpStatus::DUMP_INVALID_ARG;
    }
    if (!StrToInt(str, value)) {
        DUMPER_HILOGE(MODULE_COMMON, "StrToInt error, str=%{public}s", str.c_str());
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}

std::string CmdParse::RemoveCharacterFromStr(const std::string &str, const char character)
{
    std::string result = str;
    size_t pos;
    while ((pos = result.find(character)) != std::string::npos) {
        result.erase(pos, 1);
    }
    return result;
}

DumpStatus CmdParse::CheckProcessAlive(DumpContext& dumpContext)
{
    auto opts = dumpContext.GetDumperOpts();
    std::vector<int> pids = {
        opts->cpuUsagePid,
        opts->memPid,
        opts->processPid
    };

    for (int pid : pids) {
        if (pid < -1) {
            PrintErrorMessage(dumpContext, INVALID_ARG_MSG + std::to_string(pid));
            return DumpStatus::DUMP_FAIL;
        }
        if (pid > -1 && !DumpUtils::CheckProcessAlive(pid)) {
            PrintErrorMessage(dumpContext, PID_ERROR_MSG + std::to_string(pid));
            return DumpStatus::DUMP_FAIL;
        }
    }
    return DumpStatus::DUMP_OK;
}

DumpStatus CmdParse::VerifyOptions(const DumpContext& dumpContext)
{
    auto opts = dumpContext.GetDumperOpts();
    if (!opts->isDumpCpuFreq && !opts->isDumpCpuUsage && !opts->isDumpMem && !opts->isDumpProcesses) {
        DUMPER_HILOGE(MODULE_COMMON, "select nothing.");
        return DumpStatus::DUMP_FAIL;
    }
    return DumpStatus::DUMP_OK;
}
} // namespace HiviewDFX
} // namespace OHOS
